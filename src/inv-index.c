#include "inv-index.h"


gint
posting_pair_compare_func (gconstpointer a, gconstpointer b)
{
    PostingPair *p1 = (PostingPair *) a;
    PostingPair *p2 = (PostingPair *) b;

    gint doc_diff = (p1->doc_id < p2->doc_id ? -1 :
                     (p1->doc_id > p2->doc_id ? 1 : 0));
    gint pos_diff = (p1->pos < p2->pos ? -1 :
                     (p1->pos > p2->pos ? 1 : 0));


    return (doc_diff == 0 ? pos_diff : doc_diff);
}

PostingList *
posting_list_new (void)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    posting_list->list = NULL;

    return posting_list;
}

guint
posting_list_size (PostingList *posting_list)
{
    return g_slist_length(posting_list->list);
}

void
posting_list_add (PostingList *posting_list, gint doc_id, gint pos)
{
    PostingPair *pair = g_malloc(sizeof(PostingPair));
    pair->doc_id = doc_id;
    pair->pos    = pos;

    if (posting_list->list == NULL){
        posting_list->list = g_slist_append(NULL, pair);
    } else {
        posting_list->list = g_slist_insert_sorted(posting_list->list, pair, posting_pair_compare_func);
    }
}

GSList *
posting_list_get_slist (PostingList *posting_list)
{
    return posting_list->list;
}

InvIndex *
inv_index_new (void)
{
    InvIndex *inv_index = g_malloc(sizeof(InvIndex));

    inv_index->hash = g_hash_table_new(g_str_hash, g_str_equal);

    return inv_index;
}

int
inv_index_numterms (InvIndex *inv_index)
{
    return g_hash_table_size(inv_index->hash);
}

void
inv_index_add_term (InvIndex *inv_index, const gchar *term, gint doc_id, gint pos)
{
    PostingList *posting_list;
    GHashTable *hash_table = inv_index->hash;

    if ((posting_list = g_hash_table_lookup(hash_table, term)) == NULL){
        posting_list = posting_list_new();
        g_hash_table_insert(hash_table, (gpointer) term, posting_list);
    }

    posting_list_add(posting_list, doc_id, pos);
}

PostingList *
inv_index_get (InvIndex *inv_index, const gchar *term)
{
    return (PostingList *) g_hash_table_lookup(inv_index->hash, term);
}
