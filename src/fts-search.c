
#include <inv-index.h>

#define RECV_BLOCK_SIZE (8192/2)

#include <glib.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>

typedef struct process_env process_env_t;

static void parse_args (gint *args, gchar ***argv);
static inline ssize_t send_all(gint sockfd, const gchar *msg, ssize_t size);
static inline ssize_t recv_all(gint sockfd, gchar *res, ssize_t size);

static gchar *hostname;

#define MSG(...)                                \
    {                                           \
        if (option.verbose == TRUE) {           \
            flockfile(stderr);                  \
            fprintf(stderr, "[%s] ", hostname); \
            fprintf(stderr, __VA_ARGS__);       \
            funlockfile(stderr);                \
        }                                       \
    }

typedef struct _Query {
    guint id;
    const gchar *str;
} Query;

static struct {
    gboolean verbose;
    gboolean quit;
    FILE *output;
} option;

static GOptionEntry entries[] =
{
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &option.verbose, "send QUIT command", NULL },
    { "quit", 'q', 0, G_OPTION_ARG_NONE, &option.quit, "send QUIT command", NULL },
    { NULL }
};

void
parse_args (gint *argc, gchar ***argv)
{
    GError *error = NULL;
    GOptionContext *context;

    option.verbose = FALSE;
    option.output = stdout;
    option.quit = FALSE;

    context = g_option_context_new ("- test tree model performance");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_printerr("[%s]: option parsing failed: %s\n",
                   hostname,
                   error->message);
        exit (EXIT_FAILURE);
    }

    return;
failure:
    exit (EXIT_FAILURE);
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

struct recv_arg {
    gint sockfd;
    guint qnum;
};

void *
receiver_thread (void *ptr)
{
    struct recv_arg *arg = ptr;
    gint sockfd;
    GList *frames;
    GList *frame_cell;
    Frame *frame;
    guint qid;
    guint idx;

    qid = 1;
    sockfd = arg->sockfd;

    for (idx = 0; idx < arg->qnum; idx++){
        if ((frames = frame_recv_result(sockfd)) == NULL) {
            g_printerr("Insufficient result\n");
            break;
        }

        frame = frames->data;
        printf("## %d %d\n", qid, frame->extra_field);
        for (frame_cell = frames; frame_cell != NULL; frame_cell = frame_cell->next){
            frame = frame_cell->data;
            fwrite(&frame->body.lrr.buf, sizeof(gchar),
                   frame->content_length, stdout);
        }
        qid ++;
    }
}

void
process_query(const gchar *host,
              Query *qs,
              guint qn)
{
    gint sockfd;
    struct addrinfo hints;
    struct addrinfo *res;
    GString *child_hostname;
    gint retry;
    pthread_t receiver;
    guint idx;

    if (option.verbose){
        for(idx = 0; idx < qn; idx++){
            fprintf(stderr, "## %d %s\n", idx + 1, qs[idx].str);
        }
        fprintf(stderr, "#####################################\n");
    }

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, "30001", &hints, &res) != 0){
        g_printerr("getaddrinfo failed: hostname = %s\n", host);
        exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        g_printerr("Cannot make socket\n");
        exit(EXIT_FAILURE);
    }
    gint on = 1;
    MSG("TCP_NODELAY enabled\n");
    if (setsockopt(sockfd, IPPROTO_TCP,
                   TCP_NODELAY, (char *) &on,
                   sizeof(gint)) != 0){
        g_printerr("setsockopt failed: errno = %d\n", errno);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        // retry
        g_printerr("[%s] failed to connect: errno = %d\n",
                   hostname, errno);
        exit(EXIT_FAILURE);
    }

    if (option.quit){
        frame_send_quit(sockfd);
        g_printerr("sent QUIT command\n");
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        return;
    }

    struct recv_arg arg;
    arg.qnum = qn;
    arg.sockfd = sockfd;
    if (pthread_create(&receiver, NULL, receiver_thread, &arg) != 0){
        g_printerr("failed to make receiver thread: errno = %d\n",
                   errno);
        exit(EXIT_FAILURE);
    }

    for(idx = 0; idx < qn; idx++){
        frame_send_query(sockfd, qs[idx].str);
    }
    frame_send_bye(sockfd);
    pthread_join(receiver, NULL);
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
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

    if (argc <= 1){
        g_printerr("fts-search [options] HOSTNAME\n\n");
        exit(EXIT_FAILURE);
    } else {
        g_printerr("%s\n", argv[1]);
    }

    GString *input;
    gchar buf[4096];
    size_t sz;
    guint qnum;
    gchar *endptr;
    Query *queries;
    guint qid;
    const gchar *str;
    gchar *ptr;

    if (!option.quit){
        input = g_string_new("");
        while((sz = fread(buf, sizeof(gchar), 4096, stdin)) != 0){
            g_string_append_len(input, buf, sz);
        }
        qnum = 0;
        queries = NULL;
        endptr = input->str;

        while((qid = strtol(endptr, &endptr, 10)) > 0){
            endptr = index(endptr, '"');
            ptr = index(endptr, '\n');

            qnum++;
            queries = g_realloc(queries, sizeof(Query) * qnum);
            queries[qnum-1].id = qid;
            queries[qnum-1].str = g_strndup(endptr, ptr - endptr);
            endptr = ptr + 1;
        }
    }

    process_query(argv[1], queries, qnum);

    return 0;
}


