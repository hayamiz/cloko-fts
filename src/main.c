
#include <inv-index.h>
#include <stdlib.h>
#include <pthread.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct process_env process_env_t;

void parse_args (gint *args, gchar ***argv);
void run (void);
void *child_thread_func (void *data);
void *heartbeat_thread_func (void *data);
gchar *get_hostname (void);
void process_query(const gchar *query, process_env_t *env);

static gboolean exit_flag = FALSE;
static const gchar *hostname;
static FixedIndex *findex = NULL;
static DocumentSet *docset;

static struct {
    const gchar *datafile;
    const gchar *save_index;
    const gchar *load_index;
    const gchar *network;
    gint port;
    gboolean daemon;
    gint doc_limit;
} option;

struct process_env {
    GSocketConnection *sock;
    GAsyncQueue *queue;
    guint child_num;
};

typedef struct query_result {
    guint size;
    gchar *retstr;
} query_result_t;

static GOptionEntry entries[] =
{
    { "datafile", 'd', 0, G_OPTION_ARG_STRING, &option.datafile, "", "FILE" },
    { "save-index", 'o', 0, G_OPTION_ARG_STRING, &option.save_index, "", "FILE" },
    { "load-index", 'i', 0, G_OPTION_ARG_STRING, &option.load_index, "", "FILE" },
    { "daemon", 'D', 0, G_OPTION_ARG_NONE, &option.daemon, "", "FILE" },
    { "network", 'n', 0, G_OPTION_ARG_STRING, &option.network, "host numbers (ex. 000,001) or 'none'", "HOSTS" },
    { "port", 'p', 0, G_OPTION_ARG_INT, &option.network, "port number", "PORT" },
    { "doc-limit", 'l', 0, G_OPTION_ARG_INT, &option.doc_limit, "", "NUM" },
    { NULL }
};

typedef struct _child_thread_arg_t {
    guint id;
    const gchar *child_hostname;
    pthread_mutex_t *sock_mutex;
    GSocketConnection *sock;
    pthread_barrier_t *barrier;
    const gchar **query;
    GAsyncQueue *queue;
} child_thread_arg_t;

typedef struct _heartbeat_arg_t {
    guint child_num;
    child_thread_arg_t *args;
} heartbeat_arg_t;

void
parse_args (gint *argc, gchar ***argv)
{
    GError *error = NULL;
    GOptionContext *context;

    option.datafile = NULL;
    option.save_index = NULL;
    option.load_index = NULL;
    option.network = NULL;
    option.port    = 30001;
    option.daemon  = FALSE;
    option.doc_limit = -1;

    context = g_option_context_new ("- test tree model performance");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_printerr ("option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    if (option.datafile == NULL){
        g_printerr ("--datafile required\n");
        goto failure;
    } else {
        if (access(option.datafile, F_OK) != 0){
            g_printerr("No such file or directory: %s\n", option.datafile);
            goto failure;
        }
    }

    if (option.load_index &&
        access(option.load_index, F_OK) != 0){
        g_printerr("No such file or directory: %s\n", option.load_index);
        goto failure;
    }

    if (option.save_index != NULL && option.load_index != NULL){
        g_printerr ("cannot specify both --save-index and --load-index\n");
        goto failure;
    }

    return;
failure:
    exit (EXIT_FAILURE);
}

void
run(void)
{
    pthread_t *heartbeat_thread = NULL;
    heartbeat_arg_t hb_arg;
    pthread_t *child_threads = NULL;
    pthread_mutex_t *child_sock_mutexes = NULL;
    pthread_barrier_t barrier;
    child_thread_arg_t *child_thread_args = NULL;
    guint child_num = 0;
    gchar **child_hostnames = NULL;
    gint i;
    GAsyncQueue *queue = g_async_queue_new();
    const gchar *query;

    if (option.network != NULL && strcmp("none", option.network) != 0){
        child_hostnames = g_strsplit(option.network, ",", 0);
        for(i = 0;child_hostnames[i] != NULL;i++){
            child_num++;
        }
        pthread_barrier_init(&barrier, NULL, child_num + 1);
        child_threads = g_malloc(sizeof(pthread_t) * child_num);
        child_sock_mutexes = g_malloc(sizeof(pthread_mutex_t) * child_num);
        child_thread_args = g_malloc(sizeof(child_thread_arg_t) * child_num);
        for(i = 0;i < child_num;i++){
            child_thread_arg_t *arg = &child_thread_args[i];
            arg->id = i;
            pthread_mutex_init(&child_sock_mutexes[i], NULL);
            arg->sock_mutex = &child_sock_mutexes[i];
            arg->child_hostname = child_hostnames[i];
            arg->query = &query;
            arg->queue = queue;;
            arg->barrier = &barrier;
            pthread_create(&child_threads[i], NULL, child_thread_func, arg);
        }

        heartbeat_thread = g_malloc(sizeof(pthread_t));
        hb_arg.child_num = child_num;
        hb_arg.args = child_thread_args;
        // pthread_create(heartbeat_thread, NULL, heartbeat_thread_func, &hb_arg);
    } else {
        pthread_barrier_init(&barrier, NULL, 1);
    }

    GSocketListener *listener = g_socket_listener_new();
    g_socket_listener_add_inet_port(listener, option.port, NULL, NULL);

    g_printerr("Start listening on port %d.\n", option.port);

    GSocketConnection *sock;
    GInetSocketAddress *remote_addr;
    gchar readbuf[4096];
    GString *buf;
    GInputStream *stream;
    gssize sz;
    const gchar *ptr;
accept_again:
    sock = g_socket_listener_accept(listener, NULL, NULL, NULL);
    remote_addr = G_INET_SOCKET_ADDRESS(g_socket_connection_get_remote_address(sock, NULL));
        buf = g_string_new("");
    g_printerr("Accepted connection from %s\n",
               g_inet_address_to_string(g_inet_socket_address_get_address(remote_addr)));

    stream = g_io_stream_get_input_stream(G_IO_STREAM(sock));
    while((sz = g_input_stream_read(stream, readbuf, 4096, NULL, NULL)) != 0){
        g_string_append_len(buf, readbuf, sz);
        if ((ptr = index(buf->str, '\n')) == NULL){
            continue;
        }
        g_printerr("==== %s: received: ====\n", hostname);
    retry:
        if (strncmp("HTBT\n", buf->str, 5) == 0){
            // g_printerr("heartbeat command received.\n");
            g_string_erase(buf, 0, 5);
        } else if (strncmp("QUIT\n", buf->str, 5) == 0){
            g_printerr("==== %s: QUIT command ====\n", hostname);
            g_string_erase(buf, 0, 5);
            exit_flag = TRUE;
            pthread_barrier_wait(&barrier);

            goto quit;
        } else if (strncmp("QURY ", buf->str, 5) == 0 && index(buf->str, '\n') != NULL){
            g_printerr("==== %s: QURY command ====\n", hostname);
            g_string_erase(buf, 0, 5);
            const gchar *q_end = strstr(buf->str, "\"\n");
            query = g_strndup(buf->str, (q_end - buf->str) * sizeof(gchar));
            g_string_erase(buf, 0, ((q_end + 2) - buf->str) * sizeof(gchar));
            pthread_barrier_wait(&barrier);

            process_env_t env;
            env.sock = sock;
            env.queue = queue;
            env.child_num = child_num;
            process_query(query, &env);
        } else {
            if ((ptr = index(buf->str, '\n')) != NULL){
                g_string_erase(buf, 0, ((ptr + 1) - buf->str) * sizeof(gchar));
            }
            continue;
        }
        goto retry;
    }

    g_string_free(buf, TRUE);
    goto accept_again;
    
quit:
    g_socket_listener_close(listener);
    g_input_stream_close(stream, NULL, NULL);
    for(i = 0;i < child_num;i++){
        pthread_join(child_threads[i], NULL);
    }
}

void
process_query (const gchar *query, process_env_t *env)
{
    g_printerr("==== %s: process_query ====\n", hostname);

    query_result_t ret;
    query_result_t *tmp;
    FixedPostingList *base_list = NULL;
    FixedPostingList *tmp_list;
    GRegex *regex = g_regex_new("\"([^\"]+)\"", 0, 0, NULL);
    GMatchInfo *match_info = NULL;

    gchar **phrases = g_strsplit(query + 1, "\" \"", 0);
    guint i = 0;

    gchar *phrase_str;
    gchar **phrases_ptr = phrases;
    const gchar *term;
    while(*phrases_ptr != NULL){
        phrase_str = *phrases_ptr;
        Phrase *phrase = phrase_new();
        Tokenizer *tok = tokenizer_new(phrase_str);
        while((term = tokenizer_next(tok)) != NULL){
            phrase_append(phrase, term);
        }

        tmp_list = fixed_index_phrase_get(findex, phrase);
        base_list = fixed_posting_list_doc_intersect(base_list, tmp_list);

        phrases_ptr++;
    }

    GString *buf = g_string_new("");
    guint sz;
    PostingPair *pair;
    if (base_list){
        pair = base_list->pairs;
        ret.size = sz = fixed_posting_list_size(base_list);
        for(i = 0;i < sz;i++){
            g_string_append(buf, document_raw_record(document_set_nth(docset, pair->doc_id)));
            pair++;
        }
    } else {
        ret.size = 0;
    }

    g_printerr("==== %s: process_query: aggregating children's results ====\n", hostname);
    for(i = 0;i < env->child_num;i++){
        tmp = (query_result_t *)g_async_queue_pop(env->queue);
        ret.size += tmp->size;
        g_string_append(buf, tmp->retstr);
        g_free(tmp->retstr);
        g_free(tmp);
    }
    g_printerr("==== %s: process_query: aggregated ====\n", hostname);

    // g_print("# <query_id> %d\n", ret.size);
    // printf(buf->str);
    GString *header = g_string_new("");
    g_string_sprintf(header,
                     "RSLT %d %d\n", ret.size, buf->len);
    GOutputStream *os = g_io_stream_get_output_stream(G_IO_STREAM(env->sock));
    g_output_stream_write_all(os, header->str, header->len, NULL, NULL, NULL);
    g_output_stream_write_all(os, buf->str, buf->len, NULL, NULL, NULL);
    g_output_stream_write_all(os, "\n", 1, NULL, NULL, NULL);
    g_string_free(header, TRUE);
    g_string_free(buf, TRUE);
}

void *
child_thread_func (void *data)
{
    child_thread_arg_t *arg = (child_thread_arg_t *) data;
    GSocketConnection *conn;
    GSocketClient *client;
    GOutputStream *stream;
    GInputStream *is;
    GString *child_hostname = g_string_new("");
    g_async_queue_ref(arg->queue);
    // g_string_sprintf(child_hostname, "cloko%s.sc.iis.u-tokyo.ac.jp", arg->child_hostname);
    g_string_sprintf(child_hostname, "%s.sc.iis.u-tokyo.ac.jp", arg->child_hostname);
    client = g_socket_client_new();
    gint retry;
    pthread_mutex_lock(arg->sock_mutex);
    for(retry = 0;;retry++){
        if (retry > 100){
            g_printerr("%s: cannot connect to child %s\n",
                       hostname,
                       child_hostname->str);
            exit(EXIT_FAILURE);
        }
        conn = g_socket_client_connect_to_host(client,
                                               child_hostname->str,
                                               option.port,
                                               NULL, NULL);
        if (conn) break;
        g_usleep(1000000);
    }

    g_printerr("%s: connected to child %s\n",
               hostname,
               child_hostname->str);

    arg->sock = conn;
    pthread_mutex_unlock(arg->sock_mutex);

    GString *buf = g_string_new("");
    GString *tmpbuf = g_string_new("");
    gchar readbuf[4096];
    query_result_t *ret;
    guint sz;
    const gchar *ptr;
    gchar *endptr;
    guint bytes;
    
    stream = g_io_stream_get_output_stream(G_IO_STREAM(conn));
    is = g_io_stream_get_input_stream(G_IO_STREAM(conn));
    buf = g_string_new("");

    for(;;){
        pthread_barrier_wait(arg->barrier);
        g_printerr("%s: thread %d passed barrier.\n", hostname, arg->id);
        if (exit_flag){
            pthread_mutex_lock(arg->sock_mutex);
            g_output_stream_write_all(stream, "QUIT\n", 5, NULL, NULL, NULL);
            pthread_mutex_unlock(arg->sock_mutex);
            break;
        }

        // query arrived
        tmpbuf = g_string_new("");
        g_string_printf(tmpbuf, "QURY %s\"\n", *arg->query);
        pthread_mutex_lock(arg->sock_mutex);
        g_output_stream_write_all(stream, tmpbuf->str, tmpbuf->len, NULL, NULL, NULL);
        pthread_mutex_unlock(arg->sock_mutex);
        g_string_free(tmpbuf, TRUE);

    retry:
        while((ptr = index(buf->str, '\n')) == NULL){
            sz = g_input_stream_read(is, readbuf, 4096, NULL, NULL);
            if (sz == 0){
                // connection closed.
                exit(EXIT_FAILURE);
            }
            g_string_append_len(buf, readbuf, sz);
        }
        if (g_str_has_prefix(buf->str, "RSLT ")){
            ret = g_malloc(sizeof(query_result_t));
            ret->size = 0;
            ret->retstr = NULL;
            ret->size = strtol(buf->str + 5, &endptr, 10);
            bytes = strtol(endptr, NULL, 10);
            g_string_erase(buf, 0, ((ptr + 1) - buf->str) * sizeof(gchar));

            while(buf->len < bytes + 1){
                sz = g_input_stream_read(is, readbuf, 4096, NULL, NULL);
                if (sz == 0){
                    // connection closed.
                    exit(EXIT_FAILURE);
                }
                g_string_append_len(buf, readbuf, sz);
            }
            ret->retstr = g_strndup(buf->str, bytes);
            g_string_erase(buf, 0, bytes + 1);
            g_async_queue_push(arg->queue, ret);
        } else {
            g_string_erase(buf, 0, ((ptr + 1) - buf->str) * sizeof(gchar));
            goto retry;
        }
    }
quit:
    g_async_queue_unref(arg->queue);
}

void *
heartbeat_thread_func (void *data)
{
    heartbeat_arg_t *arg = (heartbeat_arg_t *) data;
    gint i;

    while (!exit_flag){
        GSocketConnection *sock;
        for(i = 0;i < arg->child_num;i++){
            pthread_mutex_lock(arg->args[i].sock_mutex);
            if (arg->args[i].sock == NULL){
                pthread_mutex_unlock(arg->args[i].sock_mutex);
                continue;
            }
            sock = arg->args[i].sock;
            GOutputStream *stream = g_io_stream_get_output_stream(G_IO_STREAM(sock));
            g_output_stream_write_all(stream, "HTBT\n", 5, NULL, NULL, NULL);
            pthread_mutex_unlock(arg->args[i].sock_mutex);
        }
        g_usleep(5000000);
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

    g_print("%s: start loading document %s\n", hostname, option.datafile);
    timer = g_timer_new();
    g_timer_start(timer);
    docset = document_set_load(option.datafile);
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

        Tokenizer *tok;
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
                g_print("%s: %d/%d documents indexed: %d terms.\n",
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
    }

    if (option.daemon){
        run();
    }

    return 0;
}
