
#define RECV_BLOCK_SIZE 8192

#include <inv-index.h>
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

typedef struct process_env process_env_t;

static void parse_args (gint *args, gchar ***argv);
static void run (void);
static void *child_downstreamer (void *data);
static void *child_upstreamer (void *data);
static gchar *get_hostname (void);
static void process_query(const gchar *query, process_env_t *env);
void   phrase_search_thread_func (gpointer data, gpointer user_data);
static inline ssize_t send_all(gint sockfd, const gchar *msg, ssize_t size);
static inline ssize_t recv_all(gint sockfd, gchar *res, ssize_t size);

#define MSG(...)                                \
    {                                           \
        if (option.verbose == TRUE) {           \
            flockfile(stderr);                  \
            fprintf(stderr, "[%s] ", hostname); \
            fprintf(stderr, __VA_ARGS__);       \
            funlockfile(stderr);                \
        }                                       \
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
} option;

struct process_env {
    gint sockfd;
    GAsyncQueue *queue;
    guint child_num;
    GTimer *timer;
    GThreadPool *thread_pool;
};

typedef struct phrase_search {
    const gchar *phrase_str;
    GAsyncQueue *queue;
    FixedPostingList *ret;
} phrase_search_arg_t;

typedef struct query_result {
    guint size;
    gchar *retstr;
} query_result_t;

static GOptionEntry entries[] =
{
    { "datafile", 'd', 0, G_OPTION_ARG_STRING, &option.datafile, "", "FILE" },
    { "save-index", 'o', 0, G_OPTION_ARG_STRING, &option.save_index, "", "FILE" },
    { "load-index", 'i', 0, G_OPTION_ARG_STRING, &option.load_index, "", "FILE" },
    { "daemon", 'D', 0, G_OPTION_ARG_NONE, &option.daemon, "", NULL },
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
    { NULL }
};

typedef struct _child_arg_t {
    guint id;
    const gchar *child_hostname;
    gint sockfd;
    pthread_barrier_t *barrier;
    const gchar **query;
    GAsyncQueue *queue;
} child_arg_t;

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
    option.doc_limit = -1;
    option.timeout = 100;
    option.tcp_nodelay = 0;
    option.tcp_cork = 0;
    option.relay = FALSE;
    option.verbose = FALSE;

    context = g_option_context_new ("- test tree model performance");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        MSG("option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    if (option.relay == FALSE){
        if (option.datafile == NULL){
            MSG("--datafile required\n");
            goto failure;
        } else {
            if (access(option.datafile, F_OK) != 0){
                MSG("No such file or directory: %s\n", option.datafile);
                goto failure;
            }
        }
    }

    if (option.load_index &&
        access(option.load_index, F_OK) != 0){
        MSG("No such file or directory: %s\n", option.load_index);
        goto failure;
    }

    if (option.save_index != NULL && option.load_index != NULL){
        MSG("cannot specify both --save-index and --load-index\n");
        goto failure;
    }

    return;
failure:
    exit (EXIT_FAILURE);
}

void
run(void)
{
    pthread_t *child_threads = NULL;
    pthread_barrier_t barrier;
    child_arg_t *child_thread_args = NULL;
    guint child_num = 0;
    gchar **child_hostnames = NULL;
    gint i;
    GAsyncQueue *queue = g_async_queue_new();
    const gchar *query;
    GThreadPool *thread_pool;

    thread_pool = g_thread_pool_new(phrase_search_thread_func,
                                    NULL,
                                    16,
                                    TRUE,
                                    NULL);

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
            arg->query = &query;
            arg->queue = queue;;
            arg->barrier = &barrier;
            pthread_create(&child_threads[i], NULL, child_downstreamer, arg);
        }
    } else {
        pthread_barrier_init(&barrier, NULL, 1);
    }

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
        MSG("getaddrinfo failed.\n");
        exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        MSG("socket failed: errno = %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET,
                   SO_REUSEADDR, (char * ) & on, sizeof (on)) != 0){
        MSG("setsockopt failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        MSG("bind failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, 32)){
        MSG("listen failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    MSG("Start listening on port %s.\n", option.port);

    gchar recv_block[RECV_BLOCK_SIZE];
    GString *recvbuf;
    gssize sz;
    const gchar *eolptr;
    GTimer *timer;
    gint client_sockfd;

    timer = g_timer_new();
    recvbuf = g_string_new("");

accept_again:
    if ((client_sockfd = accept(sockfd, res->ai_addr, &res->ai_addrlen)) == -1){
        MSG("accept failed: errno = %d\n", errno);
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    MSG("accepted connection\n");

    while((sz = read(client_sockfd, recv_block, RECV_BLOCK_SIZE)) > 0){
        g_string_append_len(recvbuf, recv_block, sz);
        MSG("==== received: ====\n");
    retry:
        if ((eolptr = index(recvbuf->str, '\n')) == NULL){
            continue;
        }
        if (strncmp("QUIT\n", recvbuf->str, 5) == 0){
            MSG("==== QUIT command ====\n");
            exit_flag = TRUE;
            pthread_barrier_wait(&barrier);

            goto quit;
        } else if (strncmp("BYE\n", recvbuf->str, 4) == 0){
            MSG("==== BYE command ====\n");
            sz = 0;
            break;
        } else if (strncmp("QUERY ", recvbuf->str, 6) == 0){
            guint qlen = (eolptr - (recvbuf->str + 6)) * sizeof(gchar);
            MSG("==== QUERY command ====\n");
            query = g_strndup(recvbuf->str + 6, qlen);
            pthread_barrier_wait(&barrier);

            process_env_t env;
            env.sockfd = client_sockfd;
            env.queue = queue;
            env.child_num = child_num;
            env.timer = timer;
            env.thread_pool = thread_pool;
            process_query(query, &env);
        }
        g_string_erase(recvbuf, 0, ((eolptr + 1) - recvbuf->str) * sizeof(gchar));
        goto retry;
    }
    if (sz == 0){
        g_string_set_size(recvbuf, 0);
        shutdown(client_sockfd, SHUT_RDWR);
        close(client_sockfd);
        goto accept_again;
    } else if (sz < 0) {
        MSG("invalid end of stream.\n");
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
quit:
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    freeaddrinfo(res);
    g_string_free(recvbuf, TRUE);
    for(i = 0;i < child_num;i++){
        pthread_join(child_threads[i], NULL);
    }
}

void
process_query (const gchar *query, process_env_t *env)
{
    GRegex *regex;
    GMatchInfo *match_info;
    query_result_t ret;
    query_result_t *tmp;
    FixedPostingList *base_list;
    FixedPostingList *tmp_list;
    GString *output_header;
    GString *output_body;
    guint i;
    guint sz;
    PostingPair *pair;
    gdouble time;
    Tokenizer *tok;
    GAsyncQueue *search_queue;
    guint num_phrases;
    phrase_search_arg_t *arg;

    g_timer_start(env->timer);

    output_header = g_string_new("");
    output_body = g_string_new("");
    ret.size = 0;
    search_queue = g_async_queue_new();
    num_phrases = 0;

    if (option.relay == FALSE) {
        regex = g_regex_new("\"([^\"]+)\"", 0, 0, NULL);
        base_list = NULL;
        tmp_list = NULL;
        tok = NULL;

        g_regex_match (regex, query, 0, &match_info);
        while (g_match_info_matches (match_info))
        {
            gchar *phrase_str = g_match_info_fetch (match_info, 1);
            num_phrases++;

            arg = g_malloc(sizeof(phrase_search_arg_t));
            arg->phrase_str = phrase_str;
            arg->queue = search_queue;
            g_thread_pool_push(env->thread_pool, arg, NULL);

            g_match_info_next (match_info, NULL);
        }

        for(;num_phrases > 0;num_phrases--){
            arg = g_async_queue_pop(search_queue);
            base_list = fixed_posting_list_doc_intersect(base_list,
                                                         arg->ret);
            g_free(arg);
        }

        if (base_list){
            pair = base_list->pairs;
            ret.size = sz = fixed_posting_list_size(base_list);
            for(i = 0;i < sz;i++){
                g_string_append(output_body, document_raw_record(document_set_nth(docset, pair->doc_id)));
                pair++;
            }
        }
        g_match_info_free(match_info);
        MSG("query processing time: %lf [msec]\n",
            (time = g_timer_elapsed(env->timer, NULL) * 1000));
    }
    g_timer_start(env->timer);

    MSG("process_query: aggregating children's results\n");
    for(i = 0;i < env->child_num;i++){
        tmp = (query_result_t *)g_async_queue_pop(env->queue);
        ret.size += tmp->size;
        g_string_append(output_body, tmp->retstr);
        g_free(tmp->retstr);
        g_free(tmp);
    }
    MSG("process_query: aggregated: %lf msec\n",
        (time = g_timer_elapsed(env->timer, NULL) * 1000));

    // g_print("# <query_id> %d\n", ret.size);
    // printf(buf->str);
    MSG("RESULT %d %d\n", ret.size, output_body->len);
    g_string_sprintf(output_header,
                     "RESULT %d %d\n", ret.size, output_body->len);
    if (send_all(env->sockfd, output_header->str,
                  output_header->len) <= 0) {
        MSG("send_all failed\n");
        exit(EXIT_FAILURE);
    }
    if (output_body->len > 0){
        if (send_all(env->sockfd, output_body->str,
                     output_body->len) <= 0) {
            MSG("send_all failed\n");
            exit(EXIT_FAILURE);
        }
    }
    g_string_free(output_header, TRUE);
    g_string_free(output_body, TRUE);
    MSG("==== process_query: finished ====\n");
}

void
phrase_search_thread_func (gpointer data, gpointer user_data)
{
    phrase_search_arg_t *arg = (phrase_search_arg_t *) data;
    Tokenizer *tok;
    FixedPostingList *list;
    Phrase *phrase;
    const gchar *term;

    phrase = phrase_new();
    tok = tokenizer_new(arg->phrase_str);
    while((term = tokenizer_next(tok)) != NULL){
        phrase_append(phrase, term);
    }
    list = fixed_index_phrase_get(findex, phrase);

    // result
    arg->ret = list;
    g_async_queue_push(arg->queue, arg);
    phrase_free(phrase);
    tokenizer_free(tok);
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

    child_hostname = g_string_new("");
    g_string_sprintf(child_hostname, "%s.sc.iis.u-tokyo.ac.jp", arg->child_hostname);
    bzero(&hints, sizeof(struct addrinfo));
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(child_hostname->str, option.port, &hints, &res) != 0){
        MSG("getaddrinfo failed: hostname = %s\n", child_hostname->str);
        exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        MSG("Cannot make socket\n");
        exit(EXIT_FAILURE);
    }
    if (option.tcp_nodelay == 1){
        MSG("TCP_NODELAY enabled\n");
        if (setsockopt(sockfd, IPPROTO_TCP,
                       TCP_NODELAY, (char *) &option.tcp_nodelay,
                       sizeof(gint)) != 0){
            MSG("setsockopt failed: errno = %d\n", errno);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

reconnect:
    for(retry = 0;;retry++){
        if (retry > option.timeout){
            MSG("cannot connect to child %s\n", child_hostname->str);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
            // retry
            MSG("failed to connect. will retry: errno = %d\n", errno);
            g_usleep(1000000);
            continue;
        }
        break;
    }
    MSG("connected to child %s\n", child_hostname->str);

    arg->sockfd = sockfd;
    if (pthread_create(&upstreamer, NULL, child_upstreamer, arg) != 0){
        MSG("failed to create upstreamer of %s\n", child_hostname->str);
        exit(EXIT_FAILURE);
    }

    // parent is waiting for children getting ready
    pthread_barrier_wait(arg->barrier);

    GString *tmpbuf;

    tmpbuf = g_string_new("");

    for(;;){
        pthread_barrier_wait(arg->barrier); // wait for QUERY or QUIT command
        if (exit_flag){
            send_all(sockfd, "QUIT\n", 5);
            pthread_cancel(upstreamer);
            pthread_join(upstreamer, NULL);
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            g_string_free(tmpbuf, TRUE);
            freeaddrinfo(res);
            return NULL;
        }

        // query arrived
        g_string_printf(tmpbuf, "QUERY %s\"\n", *arg->query);
        if (send_all(sockfd, tmpbuf->str, tmpbuf->len) == -1){
            shutdown(sockfd, SHUT_RDWR);
            pthread_cancel(upstreamer);
            MSG("upstreamer canceled\n");
            pthread_join(upstreamer, NULL);
            goto reconnect;
        }
        g_string_set_size(tmpbuf, 0);
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

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    arg = (child_arg_t *) data;
    g_async_queue_ref(arg->queue);

    recvbuf = g_string_new("");
    for(;;){
        while((eolptr = index(recvbuf->str, '\n')) == NULL) {
            sz = read(arg->sockfd, recv_block, RECV_BLOCK_SIZE);
            if (sz == -1){
                MSG("(%d): read(2) error: errno = %d\n",
                    arg->id, errno);
                exit(EXIT_FAILURE);
            } else if (sz == 0) {
                MSG("(%d): child_downstreamer: cannot be happened.\n",
                    arg->id);
                exit(EXIT_FAILURE);
            }
            g_string_append_len(recvbuf, recv_block, sz);
        }
        // MSG("%s(%d): received 1 line\n", arg->id);

        guint linelen = ((eolptr + 1) - recvbuf->str);

        if (strncmp(recvbuf->str, "RESULT ", 7) == 0) {
            ret = g_malloc(sizeof(query_result_t));
            ret->size = 0;
            ret->size = strtol(recvbuf->str + 7, &endptr, 10);
            bytes = strtol(endptr, NULL, 10);
            g_string_erase(recvbuf, 0, linelen);
            rest_bytes = bytes - recvbuf->len;
            // g_printerr("%s(%d): result: size=%d, bytes = %d, rest_bytes = %d, recvbuf->len = %d, orig: '%.30s'\n",
            //            hostname, arg->id, ret->size, bytes, rest_bytes, recvbuf->len, recvbuf->str);
            ret->retstr = g_malloc(sizeof(gchar) * (bytes + 1));;
            if (recvbuf->len >= bytes) {
                memcpy(ret->retstr, recvbuf->str, bytes * sizeof(gchar));
                g_string_erase(recvbuf, 0, bytes);
            } else {
                if (recvbuf->len > 0) {
                    memcpy(ret->retstr, recvbuf->str, recvbuf->len * sizeof(gchar));
                }
                sz = recv_all(arg->sockfd, ret->retstr + recvbuf->len, sizeof(gchar) * rest_bytes);
                g_string_set_size(recvbuf, 0);
                if (sz == -1){
                    // error
                    // g_printerr("%s(%d): socket error (closed?)\n",
                    //            hostname, arg->id);
                    exit(EXIT_FAILURE);
                } else if (sz != rest_bytes){
                    // error
                    // g_printerr("%s(%d): data may be lost\n",
                    //            hostname, arg->id);
                }
            }
            ret->retstr[bytes] = '\0';
            g_async_queue_push(arg->queue, ret);
        } else {
            // g_printerr("%s(%d): not RESULT: %.50s\n",
            //            hostname, arg->id, recvbuf->str);
            g_string_erase(recvbuf, 0, linelen);
        }
    }

    MSG("(%d): upstreamer exit\n", arg->id);
    g_async_queue_unref(arg->queue);
    return NULL;
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

        if (!option.load_index){
            inv_index = inv_index_new();
            guint idx;
            guint sz = document_set_size(docset);

            if (option.doc_limit > 0){
                sz = option.doc_limit;
            }

            Tokenizer *tok = NULL;
            g_timer_start(timer);
            for(idx = 0;idx < sz;idx++){
                Document *doc = document_set_nth(docset, idx);
                tok = tokenizer_renew2(tok,
                                       document_body_pointer(doc),
                                       document_body_size(doc));
                const gchar *term;
                guint pos = 0;
                gint doc_id = document_id(doc);
                if (doc_id % 5000 == 0){
                    g_printerr("%s: %d/%d documents indexed: %d terms.\n",
                               hostname,
                               doc_id,
                               document_set_size(docset),
                               inv_index_numterms(inv_index));
                }
                while((term = tokenizer_next(tok)) != NULL){
                    inv_index_add_term(inv_index, term, doc_id, pos++);
                }
                pos = 0;
                tok = tokenizer_renew(tok, document_title(doc));
                while((term = tokenizer_next(tok)) != NULL){
                    inv_index_add_term(inv_index, term, doc_id, G_MININT + (pos++));
                }
            }
            g_timer_stop(timer);
            g_print("%s: indexed: %lf [sec]\n%s: # of terms: %d\n",
                    hostname,
                    g_timer_elapsed(timer, NULL),
                    hostname,
                    inv_index_numterms(inv_index));

            g_print("%s: packing index\n", hostname);
            g_timer_start(timer);
            findex = fixed_index_new(inv_index);
            g_timer_stop(timer);
            g_print("%s: packed: %lf [sec]\n", hostname, g_timer_elapsed(timer, NULL));

            if (inv_index_numterms(inv_index) != fixed_index_numterms(findex)){
                g_print("%s: invalid packed index\n", hostname);
                exit(EXIT_FAILURE);
            }
            fixed_index_check_validity(findex);

            if (option.save_index){
                g_timer_start(timer);
                if (fixed_index_dump(findex, option.save_index) == NULL){
                    g_print("%s: failed to save index to '%s'\n",
                            hostname,
                            option.save_index);
                    exit(EXIT_FAILURE);
                }
                if (chmod(option.save_index, S_IRUSR) != 0){
                    g_print("%s: failed to chmod index '%s'\n",
                            hostname,
                            option.save_index);
                    // exit(EXIT_FAILURE);
                }
                g_timer_stop(timer);
                g_print("%s: save index: %lf [sec]\n", hostname, g_timer_elapsed(timer, NULL));
            }
            inv_index_free(inv_index);
        } else {
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
        }
    }

    if (option.daemon){
        run();
    }

    return 0;
}
