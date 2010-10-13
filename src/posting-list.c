#include "inv-index.h"


gint
posting_pair_compare_func (gconstpointer a, gconstpointer b)
{
    PostingPair p1 = (PostingPair) a;
    PostingPair p2 = (PostingPair) b;

    return (p1 < p2 ? -1 :
            (p1 > p2 ? 1 : 0));
}

PostingList *
posting_list_new (void)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    posting_list->list = g_tree_new(posting_pair_compare_func);

    return posting_list;
}

static gboolean
posting_list_copy_gtree_func (gpointer key, gpointer value, gpointer data)
{
    GTree *tree = (GTree *) data;
    g_tree_insert(tree, key, value);
    g_printerr("new tree: %d nodes.\n", g_tree_nnodes(tree));
    return FALSE;
}

PostingList *
posting_list_copy (PostingList *orig_list)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    GTree *new_tree = g_tree_new(posting_pair_compare_func);
    g_printerr("new tree: %p.\n", new_tree);
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
posting_list_add (PostingList *posting_list, gint doc_id, gint pos)
{
    PostingPair *pair = posting_list_new(doc_id, pos)
    pair->doc_id = doc_id;
    pair->pos    = pos;

    g_tree_insert(posting_list->list, pair, (gpointer) TRUE);
}


struct posting_list_nth_gtree_arg {
    guint idx;
    guint target_idx;
    PostingPair out;
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

PostingPair
posting_list_nth (PostingList *posting_list, guint idx)
{
    if (posting_list == NULL ||
        posting_list_size(posting_list) == 0 ||
        posting_list_size(posting_list) <= idx){
        return NULL;
    }

    struct posting_list_nth_gtree_arg arg;
    arg.target_idx = idx;
    arg.idx = 0;

    g_tree_foreach(posting_list->list, posting_list_nth_gtree_func, &arg);

    return arg.out;
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

    ((PostingPair)key)->pos += arg->offset;
    if (g_tree_lookup(arg->succ_tree, key) == NULL){
        arg->delete_nodes = g_slist_prepend(arg->delete_nodes, key);
    }
    ((PostingPair)key)->pos -= arg->offset;

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

