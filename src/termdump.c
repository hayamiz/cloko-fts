
#include <inv-index.h>
#include <stdlib.h>

static struct {
    gboolean quoted;
    gboolean unique;
    gint count;
    gboolean clang;
} option;

static GOptionEntry entries[] =
{
    { "quoted", 'q', 0, G_OPTION_ARG_NONE, &option.quoted, "", NULL },
    { "unique", 'u', 0, G_OPTION_ARG_NONE, &option.unique, "", NULL },
    { "count", 'c', 0, G_OPTION_ARG_INT, &option.count, "", "NUM" },
    { "clang", 'C', 0, G_OPTION_ARG_NONE, &option.clang, "", NULL },
    { NULL }
};

void
parse_args (gint *argc, gchar ***argv)
{
    GError *error = NULL;
    GOptionContext *context;

    option.quoted = FALSE;
    option.unique = FALSE;
    option.count = 0;
    option.clang = FALSE;

    context = g_option_context_new ("- termdump");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_printerr("option parsing failed: %s\n",
                   error->message);
        exit (EXIT_FAILURE);
    }

    if (option.count > 0 && !option.unique)
    {
        g_printerr("--count must be specified with --unique\n");
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
    GHashTable *hash;

    g_type_init();

    hostname = g_malloc(sizeof(gchar) * 256);
    if (gethostname(hostname, 255) != 0){
        g_printerr("failed to get hostname.\n");
        hostname = "";
    }

    parse_args(&argc, &argv);

    tok = NULL;
    timer = g_timer_new();
    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    guint count;

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
            g_hash_table_replace(hash,
                                 g_strdup(term),
                                 (gpointer)(gint64)(1 + ((gint)(gint64) g_hash_table_lookup(hash, term))));

            if (!option.unique){
                if (option.quoted)
                    printf("\"%s\"", term);
                else
                    printf(term);

                printf(" %u %d\n", doc_id, pos++);
            }
            g_free(term);
        }

        pos = 0;
        tok = tokenizer_renew(tok, document_title(doc));
        while((term = tokenizer_next(tok)) != NULL){
            g_hash_table_replace(hash,
                                 g_strdup(term),
                                 (gpointer)(gint64)(1 + ((gint)(gint64) g_hash_table_lookup(hash, term))));

            if (!option.unique){
                if (option.quoted)
                    printf("\"%s\"", term);
                else
                    printf(term);

                printf(" %u %d\n", doc_id, G_MININT + (pos++));
            }
            g_free(term);
        }
    }
    if (option.unique){
        GList *keys, *key;
        GRand *rand;
        keys = g_hash_table_get_keys(hash);
        key = keys;
        if (option.count > 0) {
            rand = g_rand_new();
            for (; option.count > 0; option.count--){
                gint32 idx = g_rand_int_range(rand, 0, g_hash_table_size(hash));
                key = g_list_nth(keys, idx);
                if (option.clang) {
                    printf("{%d, \"%s\"},\n",
                           (gint)(gint64) g_hash_table_lookup(hash, key->data),
                           (const gchar *) key->data);
                } else {
                    if (option.quoted)
                        printf("\"%s\"", (const gchar *) key->data);
                    else
                        printf("%s", (const gchar *) key->data);
                    printf(" %u\n", (gint)(gint64) g_hash_table_lookup(hash, key->data));
                }
            }
            g_rand_free(rand);
        } else {
            while(key != NULL){
                printf("%s\n", (const gchar *) key->data);
                key = key->next;
            }
        }
        g_list_free(keys);
    }
    g_timer_stop(timer);
    g_printerr("%s: indexed: %lf [sec], # of terms: %d\n",
               hostname,
               g_timer_elapsed(timer, NULL),
               g_hash_table_size(hash));

    tokenizer_free(tok);
    document_set_free(docset);
    g_timer_destroy(timer);
    g_hash_table_destroy(hash);
    return 0;
}
