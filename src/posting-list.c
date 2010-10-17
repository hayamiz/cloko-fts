#include "inv-index.h"


gint
posting_pair_compare_func (gconstpointer a, gconstpointer b)
{
    guint64 p1 = *((guint64 *) a);
    guint64 p2 = *((guint64 *) b);

    return (p1 < p2 ? -1 :
            (p1 > p2 ? 1 : 0));
}

PostingPair *
posting_pair_new (guint32 doc_id, gint32 pos)
{
    PostingPair *pair;
    pair = g_malloc(sizeof(PostingPair));
    pair->doc_id = doc_id;
    pair->pos = pos;

    return pair;
}

PostingPair *
posting_pair_free (PostingPair *pair)
{
    g_free(pair);
}

PostingList *
posting_list_new (void)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    posting_list->list = g_tree_new(posting_pair_compare_func);

    return posting_list;
}


static posting_list_free_gtree_func(gpointer key, gpointer value, gpointer data)
{
    g_free((PostingPair *) key);
}
void
posting_list_free (PostingList *posting_list, gboolean free_pairs)
{
    if (free_pairs == TRUE){
        g_tree_foreach(posting_list->list, posting_list_free_gtree_func, NULL);
    }
    g_tree_unref(posting_list->list);
    g_slice_free1(sizeof(PostingPair), posting_list);
}

static gboolean
posting_list_copy_gtree_func (gpointer key, gpointer value, gpointer data)
{
    GTree *tree = (GTree *) data;
    g_tree_insert(tree, key, value);
    return FALSE;
}

PostingList *
posting_list_copy (PostingList *orig_list)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    GTree *new_tree = g_tree_new(posting_pair_compare_func);
    g_tree_foreach(orig_list->list, posting_list_copy_gtree_func, new_tree);
    posting_list->list = new_tree;

    return posting_list;
}

guint
posting_list_size (PostingList *posting_list)
{
    return g_tree_nnodes(posting_list->list);
}

void
posting_list_add (PostingList *posting_list, guint32 doc_id, gint32 pos)
{
    PostingPair *pair = g_slice_alloc(sizeof(PostingPair)); // posting_pair_new(doc_id, pos);
    pair->doc_id = doc_id;
    pair->pos    = pos;

    g_tree_insert(posting_list->list, pair, (gpointer) TRUE);
}

static __thread PostingPair pp_tmp;
PostingPair *
posting_list_check (PostingList *posting_list, guint32 doc_id, gint32 pos)
{
    if (posting_list){
        pp_tmp.doc_id = doc_id;
        pp_tmp.pos = pos;
        return g_tree_lookup(posting_list->list, &pp_tmp);
    }
    return NULL;
}

struct posting_list_nth_gtree_arg {
    guint idx;
    guint target_idx;
    PostingPair *out;
};

static gboolean
posting_list_nth_gtree_func (gpointer key, gpointer value, gpointer data)
{
    struct posting_list_nth_gtree_arg *arg =
        (struct posting_list_nth_gtree_arg *) data;
    if (arg->idx == arg->target_idx){
        arg->out = key;
        return TRUE;
    }
    arg->idx++;
    return FALSE;
}

struct posting_list_select_successor_gtree_arg {
    GTree *succ_tree;
    guint offset;
    GSList *delete_nodes;
};
static gboolean
posting_list_select_successor_gtree_func(gpointer key, gpointer value, gpointer data)
{
    struct posting_list_select_successor_gtree_arg *arg =
        (struct posting_list_select_successor_gtree_arg *) data;

    ((PostingPair *)key)->pos += arg->offset;
    if (g_tree_lookup(arg->succ_tree, key) == NULL){
        arg->delete_nodes = g_slist_prepend(arg->delete_nodes, key);
    }
    ((PostingPair *)key)->pos -= arg->offset;

    return FALSE;
}

PostingList *
posting_list_select_successor (PostingList *base_list,
                               PostingList *successor_list,
                               guint offset)
{
    struct posting_list_select_successor_gtree_arg arg;
    arg.succ_tree = successor_list->list;
    arg.offset = offset;
    arg.delete_nodes = NULL;

    g_tree_foreach(base_list->list, posting_list_select_successor_gtree_func, &arg);

    while(arg.delete_nodes != NULL){
        g_tree_remove(base_list->list, arg.delete_nodes->data);
        arg.delete_nodes = g_slist_next(arg.delete_nodes);
    }

    return base_list;
}

