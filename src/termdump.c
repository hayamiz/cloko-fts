
#include <inv-index.h>
#include <stdlib.h>

static struct {
    gboolean quoted;
} option;

static GOptionEntry entries[] =
{
    { "quoted", 'q', 0, G_OPTION_ARG_NONE, &option.quoted, "", NULL },
    { NULL }
};

void
parse_args (gint *argc, gchar ***argv)
{
    GError *error = NULL;
    GOptionContext *context;

    option.quoted = FALSE;

    context = g_option_context_new ("- termdump");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_printerr("option parsing failed: %s\n",
                   error->message);
        exit (EXIT_FAILURE);
    }
}

gint
main (gint argc, gchar **argv)
{
    GTimer *timer;
    Tokenizer *tok;
    guint idx;
    guint sz;
    Document *doc;
    gchar *term;
    guint doc_id;
    gint pos;
    gchar *hostname;
    DocumentSet *docset;

    g_type_init();

    hostname = g_malloc(sizeof(gchar) * 256);
    if (gethostname(hostname, 255) != 0){
        g_printerr("failed to get hostname.\n");
        hostname = "";
    }

    parse_args(&argc, &argv);

    tok = NULL;
    timer = g_timer_new();

    g_printerr("%s: start loading document\n", hostname);
    g_timer_start(timer);
    docset = document_set_load("/dev/stdin", 0);
    g_timer_stop(timer);

    g_printerr("%s: document loaded: %lf [sec]\n",
               hostname, g_timer_elapsed(timer, NULL));
    g_printerr("%s: path: STDIN, # of documents: %d\n",
               hostname, document_set_size(docset));

    sz = document_set_size(docset);

    g_timer_start(timer);
    for(idx = 0;idx < sz;idx++){
        doc = document_set_nth(docset, idx);
        tok = tokenizer_renew2(tok,
                               document_body_pointer(doc),
                               document_body_size(doc));
        pos = 0;
        doc_id = document_id(doc);
        if (doc_id % 5000 == 0){
            g_printerr("%s: %d/%d documents indexed\n",
                       hostname,
                       doc_id,
                       document_set_size(docset));
        }
        while((term = tokenizer_next(tok)) != NULL){
            if (option.quoted)
                printf("\"%s\"", term);
            else
                printf(term);

            printf(" %u %d\n", doc_id, pos++);
            g_free(term);
        }

        pos = 0;
        tok = tokenizer_renew(tok, document_title(doc));
        while((term = tokenizer_next(tok)) != NULL){
            if (option.quoted)
                printf("\"%s\"", term);
            else
                printf(term);

            printf(" %u %d\n", doc_id, G_MININT + (pos++));
            g_free(term);
        }
    }
    g_timer_stop(timer);
    g_print("%s: indexed: %lf [sec]\n%s: # of terms: %d\n",
            hostname,
            g_timer_elapsed(timer, NULL),
            hostname);

    tokenizer_free(tok);
    document_set_free(docset);
    g_timer_destroy(timer);
    return 0;
}
