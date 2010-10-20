
#define RECV_BLOCK_SIZE 8192

#include <stdlib.h>
#include <pthread.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netdb.h>

#include <inv-index.h>
#include <thread-pool.h>

typedef struct process_env process_env_t;

static void parse_args (gint *args, gchar ***argv);
static void run (void);
static void standalone (void);
static void *child_downstreamer (void *data);
static void *child_upstreamer (void *data);
static gchar *get_hostname (void);
static GList *query_to_phrases (const gchar *query);
static void process_query(const gchar *query, process_env_t *env);
static void *sender_thread_func (void *data);
static inline ssize_t send_all(gint sockfd, const gchar *msg, ssize_t size);
static inline ssize_t recv_all(gint sockfd, gchar *res, ssize_t size);

#define SENDER_QUIT ((gpointer) 0x1)
#define SENDER_BYE  ((gpointer) 0x3)

#define NOT_IMPLEMENTED()                               \
    {                                                   \
        g_printerr("[%s] %s@%d: Not implemented.\n",    \
                   hostname, __FILE__, __LINE__);       \
        exit(EXIT_FAILURE);                             \
    }

#define MSG(...)                                        \
    {                                                   \
        if (option.verbose == TRUE) {                   \
            flockfile(stderr);                          \
            fprintf(stderr, "[%s] %s@%d: ",             \
                    hostname, __FILE__, __LINE__);      \
            fprintf(stderr, __VA_ARGS__);               \
            funlockfile(stderr);                        \
        }                                               \
    }

#define NOTICE(...)                             \
    {                                           \
        flockfile(stderr);                      \
        fprintf(stderr, "[%s] %s@%d: ",         \
                hostname, __FILE__, __LINE__);  \
        fprintf(stderr, __VA_ARGS__);           \
        funlockfile(stderr);                    \
    }

#define FATAL(...)                              \
    {                                           \
        flockfile(stderr);                      \
        fprintf(stderr, "[%s] %s@%d: ",         \
                hostname, __FILE__, __LINE__);  \
        fprintf(stderr, __VA_ARGS__);           \
        funlockfile(stderr);                    \
        exit(EXIT_FAILURE);                     \
    }

static gboolean exit_flag = FALSE;
static gchar *hostname;
static FixedIndex *findex = NULL;
static DocumentSet *docset;

static struct {
    const gchar *datafile;
    const gchar *save_index;
    const gchar *load_index;
    const gchar *network;
    const gchar *port;
    gboolean daemon;
    gboolean relay;
    gint doc_limit;
    gint timeout;
    gint tcp_nodelay;
    gint tcp_cork;
    gboolean verbose;
    gboolean standalone;
    gint parallel;
} option;

struct process_env {
    gint sockfd;
    GAsyncQueue *queue;
    GAsyncQueue *sender_queue;
    pthread_barrier_t *send_barrier;
    guint child_num;
    GTimer *timer;
    ThreadPool *thread_pool;
};

typedef struct phrase_search {
    const gchar *phrase_str;
    GAsyncQueue *queue;
    FixedPostingList *ret;
} phrase_search_arg_t;

typedef struct query_result {
    GList *frames;
} query_result_t;

static GOptionEntry entries[] =
{
    { "datafile", 'd', 0, G_OPTION_ARG_STRING, &option.datafile, "", "FILE" },
    { "save-index", 'o', 0, G_OPTION_ARG_STRING, &option.save_index, "", "FILE" },
    { "load-index", 'i', 0, G_OPTION_ARG_STRING, &option.load_index, "", "FILE" },
    { "daemon", 'D', 0, G_OPTION_ARG_NONE, &option.daemon, "", NULL },
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &option.standalone, "", NULL },
    { "relay", 'r', 0, G_OPTION_ARG_NONE, &option.relay, "", NULL },
    { "network", 'n', 0, G_OPTION_ARG_STRING, &option.network, "host numbers (ex. 000,001) or 'none'", "HOSTS" },
    { "timeout", 't', 0, G_OPTION_ARG_INT, &option.timeout, "Client connect timeout", "SECONDS" },
    { "port", 'p', 0, G_OPTION_ARG_STRING, &option.port, "port number", "PORT" },
    { "tcp-nodelay", 'Z', 0, G_OPTION_ARG_INT, &option.tcp_nodelay,
      "TCP_NODELAY: 0 -> disable, 1 -> enable (default: disabled)", "VAL" },
    { "tcp-cork", 'Z', 0, G_OPTION_ARG_INT, &option.tcp_cork,
      "TCP_CORK: 0 -> disable, 1 -> enable (default: disabled)", "VAL" },
    { "doc-limit", 'l', 0, G_OPTION_ARG_INT, &option.doc_limit, "", "NUM" },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &option.verbose, "", NULL },
    { "parallel", 'P', 0, G_OPTION_ARG_INT, &option.parallel, "", "NUM" },
    { NULL }
};

typedef struct _child_arg_t {
    guint id;
    const gchar *child_hostname;
    gint sockfd;
    pthread_barrier_t *barrier;
    GAsyncQueue *queue;
    GAsyncQueue *qqueue;
} child_arg_t;

typedef struct _sender_arg {
    GList *frames;
} sender_arg_t;

void
parse_args (gint *argc, gchar ***argv)
{
    GError *error = NULL;
    GOptionContext *context;

    option.datafile = NULL;
    option.save_index = NULL;
    option.load_index = NULL;
    option.network = NULL;
    option.port    = "30001";
    option.daemon  = FALSE;
    option.doc_limit = 0;
    option.timeout = 100;
    option.tcp_nodelay = 0;
    option.tcp_cork = 0;
    option.relay = FALSE;
    option.verbose = FALSE;
    option.parallel = 16;

    context = g_option_context_new ("- test tree model performance");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_printerr("[%s]: option parsing failed: %s\n",
                   hostname,
                   error->message);
        exit (EXIT_FAILURE);
    }

    if (option.relay == FALSE){
        if (option.datafile == NULL){
            g_printerr("[%s]: --datafile required\n", hostname);
            goto failure;
        } else {
            if (access(option.datafile, F_OK) != 0){
                g_printerr("[%s]: No such file or directory: %s\n", hostname, option.datafile);
                goto failure;
            }
        }
    }

    if (option.standalone && option.daemon){
        g_printerr("[%s]: cannot specify both --standalone and --daemon\n",
                   hostname);
        goto failure;
    }

    if (option.load_index &&
        access(option.load_index, F_OK) != 0){
        g_printerr("[%s]: No such file or directory: %s\n", hostname, option.load_index);
        goto failure;
    }

    if (option.save_index != NULL && option.load_index != NULL){
        g_printerr("[%s]: cannot specify both --save-index and --load-index\n",
                   hostname);
        goto failure;
    }

    if (option.doc_limit > 0 && option.load_index != NULL){
        g_printerr("[%s]: cannot specify both --doc-limit-index and --load-index\n",
                   hostname);
        goto failure;
    }

    g_option_context_free(context);

    return;
failure:
    exit (EXIT_FAILURE);
}

void
run(void)
{
    pthread_t *child_threads = NULL;
    pthread_t sender_thread;
    pthread_barrier_t barrier;
    pthread_barrier_t send_barrier;
    child_arg_t *child_thread_args = NULL;
    guint child_num = 0;
    gchar **child_hostnames = NULL;
    gint i;
    GAsyncQueue *qqueue = g_async_queue_new();
    GAsyncQueue *queue = g_async_queue_new();
    GAsyncQueue *sender_queue = g_async_queue_new();
    gchar *query;
    ThreadPool *thread_pool;

    thread_pool = fixed_index_make_thread_pool(option.parallel);

    if (option.network != NULL && strcmp("none", option.network) != 0){
        child_hostnames = g_strsplit(option.network, ",", 0);
        for(i = 0;child_hostnames[i] != NULL;i++){
            child_num++;
        }
        pthread_barrier_init(&barrier, NULL, child_num + 1);
        child_threads = g_malloc(sizeof(pthread_t) * child_num);
        child_thread_args = g_malloc(sizeof(child_arg_t) * child_num);
        for(i = 0;i < child_num;i++){
            child_arg_t *arg = &child_thread_args[i];
            arg->id = i;
            arg->child_hostname = child_hostnames[i];
            arg->queue = queue;
            arg->qqueue = qqueue;
            arg->barrier = &barrier;
            pthread_create(&child_threads[i], NULL, child_downstreamer, arg);
        }
    } else {
        pthread_barrier_init(&barrier, NULL, 1);
    }

    pthread_barrier_init(&send_barrier, NULL, 2);

    // wait children
    pthread_barrier_wait(&barrier);

    gint sockfd;
    gint parent_sockfd;
    struct addrinfo hints;
    struct addrinfo *res;
    gint on = 1;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_PASSIVE;
    if (getaddrinfo(NULL, option.port, &hints, &res) != 0){
        FATAL("getaddrinfo failed.\n");
    }
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        FATAL("socket failed: errno = %d\n", errno);
    }
    if (setsockopt(sockfd, SOL_SOCKET,
                   SO_REUSEADDR, (char * ) & on, sizeof (on)) != 0){
        close(sockfd);
        FATAL("setsockopt failed.\n");
    }
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        close(sockfd);
        FATAL("bind failed.\n");
    }
    if (listen(sockfd, 32)){
        close(sockfd);
        FATAL("listen failed.\n");
    }
    g_printerr("[%s] Start listening on port %s.\n",
               hostname, option.port);

    gchar recv_block[RECV_BLOCK_SIZE];
    gssize sz;
    const gchar *eolptr;
    GTimer *timer;
    gint client_sockfd;
    Frame *frame;
    process_env_t env;
    timer = g_timer_new();
    frame = frame_new();
    env.queue = queue;
    env.sender_queue = sender_queue;
    env.child_num = child_num;
    env.timer = timer;
    env.thread_pool = thread_pool;
    env.send_barrier = &send_barrier;

    if (pthread_create(&sender_thread, NULL, sender_thread_func, &env) != 0){
        FATAL("pthread create failed\n");
    }

accept_again:
    if ((client_sockfd = accept(sockfd, res->ai_addr, &res->ai_addrlen)) == -1){
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        FATAL("accept failed: errno = %d\n", errno);
    }
    MSG("accepted connection\n");
    env.sockfd = client_sockfd;

    while((sz = frame_recv(client_sockfd, frame)) > 0){
        switch (frame_type(frame)) {
        case FRM_QUERY:
            query = g_strndup(frame->body.q.buf,
                              frame->body.q.length);
            // TODO: this 'query' causes memory leaks
            // pthread_barrier_wait(&barrier);
            guint idx;
            for (idx = 0; idx < child_num; idx++) {
                g_async_queue_push(qqueue, g_strdup(query));
            }

            process_query(query, &env);
            g_free(query);
            break;
        case FRM_LONG_QUERY_FIRST:
            NOT_IMPLEMENTED();
            break;
        case FRM_LONG_QUERY_REST:
            NOT_IMPLEMENTED();
            break;
        case FRM_RESULT:
            NOT_IMPLEMENTED();
            break;
        case FRM_LONG_RESULT_FIRST:
            NOT_IMPLEMENTED();
            break;
        case FRM_LONG_RESULT_REST:
            NOT_IMPLEMENTED();
            break;
        case FRM_BYE:
            MSG("BYE\n");
            sz = 0;
            g_async_queue_push(sender_queue, (gpointer) SENDER_BYE);
            pthread_barrier_wait(&send_barrier);
            shutdown(client_sockfd, SHUT_RDWR);
            close(client_sockfd);
            NOTICE("close connection with BYE command. Accept again.\n");
            goto accept_again;
            break;
        case FRM_QUIT:
            MSG("QUIT\n");
            exit_flag = TRUE;
            // push dummy data to wake up sender thread
            g_async_queue_push(sender_queue, (gpointer) SENDER_QUIT);
            g_async_queue_push(qqueue, (gpointer) SENDER_QUIT);
            // pthread_barrier_wait(&barrier);
            MSG("close connection with QUIT command.\n");
            goto quit;
            break;
        }
    }
    if (sz == 0){
    } else if (sz < 0) {
        MSG("invalid end of stream.\n");
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (!exit_flag)
        goto accept_again;
quit:
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    freeaddrinfo(res);
    for(i = 0;i < child_num;i++){
        pthread_join(child_threads[i], NULL);
    }
    thread_pool_destroy(thread_pool);
    g_async_queue_unref(queue);
    g_async_queue_unref(sender_queue);
    g_timer_destroy(timer);
}

static GList *
query_to_phrases (const gchar *query)
{
    GList *phrases;
    GRegex *regex;
    GMatchInfo *match_info;
    const gchar *phrase_str;

    phrases = NULL;
    regex = g_regex_new("\"([^\"]+)\"", 0, 0, NULL);

    g_regex_match (regex, query, 0, &match_info);
    while (g_match_info_matches (match_info))
    {
        phrase_str = g_match_info_fetch (match_info, 1);
        phrases = g_list_prepend(phrases, phrase_from_string(phrase_str));
        g_match_info_next (match_info, NULL);
    }
    g_regex_unref(regex);
    g_match_info_free(match_info);

    return phrases;
}

void
process_query (const gchar *query, process_env_t *env)
{
    query_result_t *qres;
    FixedPostingList *base_list;
    FixedPostingList *tmp_list;
    FixedPostingList *fplist;
    guint i;
    guint sz;
    PostingPair *pair;
    gdouble time;
    GList *phrases;

    GList *frames;
    GList *frame_cell;
    GList *frames_head;
    GList *last_frame;
    Frame *frame;

    phrases = NULL;
    base_list = NULL;
    tmp_list = NULL;
    frames = NULL;

    g_timer_start(env->timer);
    if (option.relay == FALSE) {
        phrases = query_to_phrases(query);
        fplist = fixed_index_multithreaded_multiphrase_get(findex, env->thread_pool, phrases);
        frames = g_list_concat(frames,
                               frame_make_result_from_fixed_posting_list(docset, fplist));
        frames_head = frames;
        last_frame = g_list_last(frames);
        frames = last_frame;
        MSG("query processing time: %lf [msec]\n",
            (time = g_timer_elapsed(env->timer, NULL) * 1000));
    }

    g_timer_start(env->timer);
    MSG("process_query: aggregating children's results\n");
    for(i = 0;i < env->child_num;i++){
        qres = (query_result_t *) g_async_queue_pop(env->queue);
        if (qres->frames == NULL)
            continue;
        last_frame = g_list_last(qres->frames);
        frames = g_list_concat(frames, qres->frames);
        if (!frames_head) {
            frames_head = frames;
        }
        frames = last_frame;
        g_free(qres);
    }
    // remove 0 hit frames
    for (frame_cell = frames_head; frame_cell != NULL; frame_cell = frame_cell->next) {
        frame = frame_cell->data;
        if (frame->type == FRM_RESULT && frame->extra_field == 0){
            if (frames_head == frame_cell)
                frames_head = frame_cell = g_list_remove_link(frame_cell, frame_cell);
            else
                frame_cell = g_list_remove_link(frame_cell, frame_cell);
            if (! frame_cell) break;
        }
    }
    MSG("process_query: aggregated: %lf msec\n",
        (time = g_timer_elapsed(env->timer, NULL) * 1000));
    sender_arg_t *arg;
    arg = g_malloc(sizeof(sender_arg_t));
    arg->frames = frames_head;
    g_async_queue_push(env->sender_queue, arg);
    MSG("process_query: finished\n");
}

static void *
sender_thread_func (void *data)
{
    process_env_t *env;
    GList *frames;
    GList *frame_cell;
    sender_arg_t *arg;
    gssize sz;

    env = data;
    g_async_queue_ref(env->sender_queue);
    sz = 0;
    for(;;) {
        arg = g_async_queue_pop(env->sender_queue);
        if (arg == SENDER_QUIT) {
            MSG("sender catched QUIT command\n");
            break;
        } else if (arg == SENDER_BYE) {
            MSG("sender catched BYE command\n");
            pthread_barrier_wait(env->send_barrier);
            continue;
        }
        frames = arg->frames;
        g_free(arg);

        sz = frame_send_multi_results(env->sockfd, frames);
        if (sz == 0){
            FATAL("Connection seems to be closed\n");
        }
        if (sz < 0 ||
            (frames != NULL && sz != g_list_length(frames) * FRAME_SIZE) ||
            (frames == NULL && sz != FRAME_SIZE)){
            FATAL("failed sending result: # of frames = %d\n",
                  g_list_length(frames));
        }
        for (frame_cell = frames; frame_cell != NULL; frame_cell = frame_cell->next){
            frame_free(frame_cell->data);
        }
        g_list_free(frames);
    }
    g_async_queue_unref(env->sender_queue);
}

static inline ssize_t
send_all (gint sockfd, const gchar *msg, ssize_t size)
{
    ssize_t sz;
    ssize_t sent = 0;
    while(sent < size){
        if ((sz = write(sockfd, msg, size)) == -1) {
            return sz;
        } else if (sz == 0) {
            break;
        }
        sent += sz;
        msg += sz;
    }

    return sent;
}

static inline ssize_t
recv_all (gint sockfd, gchar *res, ssize_t size)
{
    ssize_t sz;
    ssize_t recved = 0;
    while(recved < size){
        if ((sz = read(sockfd, res, size)) == -1){
            return sz;
        } else if (sz == 0){
            break;
        }
        recved += sz;
        res += sz;
    }

    return recved;
}


void *
child_downstreamer (void *data)
{
    child_arg_t *arg = (child_arg_t *) data;
    gint sockfd;
    struct addrinfo hints;
    struct addrinfo *res;
    GString *child_hostname;
    gint retry;
    pthread_t upstreamer;
    Frame quit_frame;
    gchar *query;

    frame_make_quit(&quit_frame);
    child_hostname = g_string_new("");
    g_string_sprintf(child_hostname, "%s.sc.iis.u-tokyo.ac.jp", arg->child_hostname);
    bzero(&hints, sizeof(struct addrinfo));
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    g_async_queue_ref(arg->qqueue);

    if (getaddrinfo(child_hostname->str, option.port, &hints, &res) != 0){
        FATAL("getaddrinfo failed: hostname = %s\n", child_hostname->str);
        exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        FATAL("Cannot make socket\n");
        exit(EXIT_FAILURE);
    }
    if (option.tcp_nodelay == 1){
        MSG("TCP_NODELAY enabled\n");
        if (setsockopt(sockfd, IPPROTO_TCP,
                       TCP_NODELAY, (char *) &option.tcp_nodelay,
                       sizeof(gint)) != 0){
            close(sockfd);
            FATAL("setsockopt failed: errno = %d\n", errno);
        }
    }

reconnect:
    for(retry = 0;;retry++){
        if (retry > option.timeout){
            close(sockfd);
            FATAL("cannot connect to child %s\n",
                   child_hostname->str);
        }
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
            // retry
            g_printerr("[%s] failed to connect. will retry: errno = %d\n",
                       hostname, errno);
            g_usleep(1000000);
            continue;
        }
        break;
    }
    NOTICE("connected to child %s\n", child_hostname->str);

    arg->sockfd = sockfd;
    if (pthread_create(&upstreamer, NULL, child_upstreamer, arg) != 0){
        FATAL("failed to create upstreamer of %s\n", child_hostname->str);
        exit(EXIT_FAILURE);
    }

    // parent is waiting for children getting ready
    pthread_barrier_wait(arg->barrier);

    for(;;){
        // wait for QUERY or QUIT command arrival
        query = g_async_queue_pop(arg->qqueue);
        if (exit_flag) {
            if (frame_send (sockfd, &quit_frame) < 0){
                FATAL("failed sending QUIT command to %s\n",
                    child_hostname->str);
            }
            pthread_cancel(upstreamer);
            pthread_join(upstreamer, NULL);
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            freeaddrinfo(res);
            g_async_queue_unref(arg->qqueue);
            return NULL;
        }

        // query arrived
        if (frame_send_query(sockfd, query) < 0){
            shutdown(sockfd, SHUT_RDWR);
            pthread_cancel(upstreamer);
            NOTICE("failed sending query to %d\n",
                   child_hostname->str);
            pthread_join(upstreamer, NULL);
            goto reconnect;
        }
        g_free(query);
    }
}

void *
child_upstreamer (void *data)
{
    child_arg_t *arg;
    GString *recvbuf;
    gchar recv_block[RECV_BLOCK_SIZE];
    const gchar *eolptr;
    gchar *endptr;
    query_result_t *ret;
    gint bytes;
    gint rest_bytes;
    ssize_t sz;
    GList *frames;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    arg = (child_arg_t *) data;
    g_async_queue_ref(arg->queue);

    while ((frames = frame_recv_result(arg->sockfd)) != NULL){
        ret = g_malloc(sizeof(query_result_t));
        ret->frames = frames;
        g_async_queue_push(arg->queue, ret);
    }

    MSG("(%d): upstreamer exit\n", arg->id);
    g_async_queue_unref(arg->queue);
    return NULL;
}


void
standalone (void)
{
    gchar buf[4096 + 1];
    size_t sz;
    gchar *query;
    gchar *ptr;
    process_env_t env;

    while(fgets(buf, 4096, stdin) != NULL){
        sz = strlen(buf);
        g_return_if_fail(sz > 0 && buf[sz - 1] == '\n');
        g_return_if_fail(query = index(buf, '"'));
        for (ptr = buf + sz; TRUE; ptr--){
            if (*(ptr - 1) == '"') {
                query = g_strndup(query, ptr - query);
                break;
            }
            g_return_if_fail(ptr - 1 != buf);
        }

        // search query
    }
}


gint
main (gint argc, gchar **argv)
{
#ifndef G_THREADS_ENABLED
    puts("GLib doesn't support threads. exit");
    exit(EXIT_FAILURE);
#endif
    g_type_init();

    hostname = g_malloc(sizeof(gchar) * 256);
    if (gethostname(hostname, 255) != 0){
        g_printerr("failed to get hostname.\n");
        hostname = "";
    }

    parse_args(&argc, &argv);

    InvIndex *inv_index;
    GTimer *timer;
    guint idx;

    if (option.relay == FALSE) {
        g_print("%s: start loading document %s\n", hostname, option.datafile);
        timer = g_timer_new();
        g_timer_start(timer);
        docset = document_set_load(option.datafile, option.doc_limit);
        g_timer_stop(timer);

        g_print("%s: document loaded: %lf [sec]\n", hostname, g_timer_elapsed(timer, NULL));
        g_print("%s: path: %s, # of documents: %d\n",
                hostname, option.datafile, document_set_size(docset));

        if (option.load_index) {
            g_timer_start(timer);
            if ((findex = fixed_index_load(option.load_index)) == NULL){
                g_print("%s: failed to load index from '%s'\n",
                        hostname,
                        option.load_index);
                exit(EXIT_FAILURE);
            }
            g_timer_stop(timer);
            g_print("%s: load index: %lf [sec]\n", hostname, g_timer_elapsed(timer, NULL));
            fixed_index_check_validity(findex);
        } else {
            inv_index = inv_index_new();

            g_timer_start(timer);
            document_set_indexing_show_progress(TRUE);
            findex = document_set_make_fixed_index(docset, option.doc_limit);
            g_timer_stop(timer);
            g_print("%s: indexed: %lf [sec]\n%s: # of terms: %d\n",
                    hostname,
                    g_timer_elapsed(timer, NULL),
                    hostname,
                    fixed_index_numterms(findex));

            fixed_index_check_validity(findex);

            if (option.save_index){
                g_timer_start(timer);
                if (fixed_index_dump(findex, option.save_index) == NULL){
                    g_print("%s: failed to save index to '%s'\n",
                            hostname,
                            option.save_index);
                    exit(EXIT_FAILURE);
                }
                /* if (chmod(option.save_index, S_IRUSR) != 0){ */
                /*     g_print("%s: failed to chmod index '%s'\n", */
                /*             hostname, */
                /*             option.save_index); */
                /*     // exit(EXIT_FAILURE); */
                /* } */
                g_timer_stop(timer);
                g_print("%s: save index: %lf [sec]\n", hostname, g_timer_elapsed(timer, NULL));
            }
            inv_index_free(inv_index);
        }

        g_timer_destroy(timer);
    }

    if (option.daemon){
        run();
    } else if (option.standalone) {
        standalone();
    }

    return 0;
}
