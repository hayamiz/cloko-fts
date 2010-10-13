
#include "inv-index.h"

static gboolean
fixed_posting_list_new_gtree_func(gpointer key, gpointer value, gpointer data)
{
    PostingPair **pair = (PostingPair **) data;
    **pair = *((PostingPair *)key);
    (*pair)++;

    return FALSE;
}

FixedPostingList *
fixed_posting_list_new (PostingList *list)
{
    FixedPostingList *fplist;
    PostingPair *tmp;
    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = posting_list_size(list);
    fplist->pairs = g_malloc(sizeof(PostingPair) * fplist->size);

    tmp = fplist->pairs;
    g_tree_foreach(list->list, fixed_posting_list_new_gtree_func, &tmp);

    return fplist;
}

void
fixed_posting_list_free (FixedPostingList *fplist)
{
    if (!fplist) return;
    g_free(fplist->pairs);
    g_free(fplist);
}

FixedPostingList *
fixed_posting_list_copy  (FixedPostingList *fplist)
{
    if (!fplist) return NULL;

    FixedPostingList *new_fplist;
    new_fplist = g_malloc(sizeof(FixedPostingList));
    new_fplist->size = fixed_posting_list_size(fplist);
    new_fplist->pairs = g_malloc(sizeof(PostingPair) * new_fplist->size);
    memcpy(new_fplist->pairs, fplist->pairs, new_fplist->size * sizeof(PostingPair));

    return new_fplist;
}

guint
fixed_posting_list_size  (FixedPostingList *fplist)
{
    return (fplist ? fplist->size : 0);
}

PostingPair *
fixed_posting_list_check (FixedPostingList *fplist, guint32 doc_id, gint32 pos)
{
    if (!fplist) return NULL;

    PostingPair *pair;
    PostingPair *sentinel = fplist->pairs + fplist->size;
    for(pair = fplist->pairs;pair < sentinel;pair++){
        if (pair->doc_id == doc_id && pair->pos == pos){
            return pair;
        }
    }

    return NULL;
}

FixedPostingList *
fixed_posting_list_select_successor (FixedPostingList *base_list,
                                     FixedPostingList *succ_list,
                                     guint offset)
{
    PostingPair *p1, *p2;
    PostingPair *p1_sentinel, *p2_sentinel;
    FixedPostingList *fplist;
    guint size = 0;
    PostingPair *pairs = NULL;

    p1 = base_list->pairs;
    p2 = succ_list->pairs;
    p1_sentinel = base_list->pairs + base_list->size;
    p2_sentinel = succ_list->pairs + succ_list->size;

    while(p1 != p1_sentinel && p2 != p2_sentinel){
        p1->pos += offset;
        if (posting_pair_compare_func(p1, p2) == 0){
            p1->pos -= offset; // restore
            size++;
            pairs = g_realloc(pairs, size * sizeof(PostingPair));
            pairs[size - 1] = *p1;
            p1++; p2++;
        } else if (posting_pair_compare_func(p1, p2) < 0) {
            p1->pos -= offset; // restore
            p1++;
        } else {
            p1->pos -= offset; // restore
            p2++;
        }
    }

    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = size;
    fplist->pairs = pairs;

    return fplist;
}


