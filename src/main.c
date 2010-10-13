
#include <inv-index.h>
#include <stdlib.h>
#include <pthread.h>
#include <gio/gio.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

void parse_args (gint *args, gchar ***argv);
void run (void);
void *child_thread_func (void *data);
void *heartbeat_thread_func (void *data);

static gboolean exit_flag = FALSE;

static struct {
    const gchar *datafile;
    const gchar *save_index;
    const gchar *load_index;
    const gchar *network;
    gint port;
} option;

static GOptionEntry entries[] =
{
    { "datafile", 'd', 0, G_OPTION_ARG_STRING, &option.datafile, "", "FILE" },
    { "save-index", 'o', 0, G_OPTION_ARG_STRING, &option.save_index, "", "FILE" },
    { "load-index", 'i', 0, G_OPTION_ARG_STRING, &option.load_index, "", "FILE" },
    { "network", 'n', 0, G_OPTION_ARG_STRING, &option.network, "host numbers (ex. 000,001) or 'none'", "HOSTS" },
    { "port", 'p', 0, G_OPTION_ARG_INT, &option.network, "port number", "PORT" },
    { NULL }
};

typedef struct _child_thread_arg_t {
    guint id;
    const gchar *child_host_id;
    pthread_mutex_t *sock_mutex;
    GSocketConnection *sock;
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
    child_thread_arg_t *child_thread_args = NULL;
    guint child_num = 0;
    gchar **child_ids = NULL;
    gint i;

    if (option.network != NULL && strcmp("none", option.network) != 0){
        child_ids = g_strsplit(option.network, ",", 0);
        for(i = 0;child_ids[i] != NULL;i++){
            child_num++;
        }
        child_threads = g_malloc(sizeof(pthread_t) * child_num);
        child_sock_mutexes = g_malloc(sizeof(pthread_mutex_t) * child_num);
        child_thread_args = g_malloc(sizeof(child_thread_arg_t) * child_num);
        for(i = 0;i < child_num;i++){
            child_thread_arg_t *arg = &child_thread_args[i];
            arg->id = i;
            pthread_mutex_init(&child_sock_mutexes[i], NULL);
            arg->sock_mutex = &child_sock_mutexes[i];
            arg->child_host_id = child_ids[i];
            pthread_create(&child_threads[i], NULL, child_thread_func, arg);
        }

        heartbeat_thread = g_malloc(sizeof(pthread_t));
        hb_arg.child_num = child_num;
        hb_arg.args = child_thread_args;
        pthread_create(heartbeat_thread, NULL, heartbeat_thread_func, &hb_arg);
    }

    GSocketListener *listener = g_socket_listener_new();
    g_socket_listener_add_inet_port(listener, option.port, NULL, NULL);

    g_printerr("Start listening on port %d.\n", option.port);

    GSocketConnection *sock = g_socket_listener_accept(listener, NULL, NULL, NULL);
    GInetSocketAddress *remote_addr =
        G_INET_SOCKET_ADDRESS(g_socket_connection_get_remote_address(sock, NULL));
    GInputStream *stream;
    gchar readbuf[4096];
    GString *buf = g_string_new("");
    gssize sz;
    g_printerr("Accepted connection from %s\n",
               g_inet_address_to_string(g_inet_socket_address_get_address(remote_addr)));
    g_socket_listener_close(listener);

    stream = g_io_stream_get_input_stream(G_IO_STREAM(sock));
    while((sz = g_input_stream_read(stream, readbuf, 4096, NULL, NULL)) != 0){
        g_string_append_len(buf, readbuf, sz);
        if (strncmp("HTBT\n", buf->str, 5) == 0){
            g_printerr("heartbeat command received.\n");
        } else if (strncmp("QUIT\n", buf->str, 5) == 0){
            g_printerr("quit command received.\n");
            break;
        }
    }
    g_input_stream_close(stream, NULL, NULL);
}



void *
child_thread_func (void *data)
{
    child_thread_arg_t *arg = (child_thread_arg_t *) data;
    GSocketConnection *conn;
    GSocketClient *client;
    GOutputStream *stream;
    GString *hostname = g_string_new("");
    g_string_sprintf(hostname, "cloko%s.sc.iis.u-tokyo.ac.jp", arg->child_host_id);
    client = g_socket_client_new();
    
    gint retry;
    pthread_mutex_lock(arg->sock_mutex);
    for(retry = 0;;retry++){
        if (retry > 5){
            exit(EXIT_FAILURE);
        }
        conn = g_socket_client_connect_to_host(client,
                                               hostname->str,
                                               option.port,
                                               NULL, NULL);
        if (conn) break;
        g_usleep(1000000);
    }

    arg->sock = conn;
    pthread_mutex_unlock(arg->sock_mutex);

    g_usleep(1000000);

    pthread_mutex_lock(arg->sock_mutex);
    stream = g_io_stream_get_output_stream(G_IO_STREAM(conn));
    g_output_stream_write_all(stream, "QUIT\n", 5, NULL, NULL, NULL);
    pthread_mutex_unlock(arg->sock_mutex);
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

    parse_args(&argc, &argv);

    run();

    return 0;

    DocumentSet *docset = document_set_load(argv[1]);
    InvIndex *inv_index = inv_index_new();
    g_print("== docset ==\n  path: %s\n  # of documents: %d\n",
            argv[1], document_set_size(docset));

    guint idx;
    guint sz = document_set_size(docset);
    for(idx = 0;idx < sz;idx++){
        Document *doc = document_set_nth(docset, idx);
        Tokenizer *tok = tokenizer_new2(document_body_pointer(doc),
                                        document_body_size(doc));
        const gchar *term;
        guint pos = 0;
        gint doc_id = document_id(doc);
        if (doc_id % 1000 == 0){
            g_print("%d documents indexed: %d terms.\n",
                    doc_id,
                    inv_index_numterms(inv_index));
        }
        while((term = tokenizer_next(tok)) != NULL){
            inv_index_add_term(inv_index, term, doc_id, pos++);
        }

        tokenizer_free(tok);
    }

    g_print("indexed.\n");
    inv_index_free(inv_index);

    return 0;
}
