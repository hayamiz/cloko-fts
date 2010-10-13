
#include <inv-index.h>


gint
main (gint argc, const gchar **argv)
{
    if (argc < 2){
        return 1;
    }

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
    }

    g_print("indexed.\n");

    return 0;
}
