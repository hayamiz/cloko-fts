
#include "inv-index.h"

struct fixed_posting_list_new_rec {
    guint idx;
    FixedPostingList *fplist;
};
static gboolean
fixed_posting_list_new_gtree_func(gpointer key, gpointer value, gpointer data)
{
    struct fixed_posting_list_new_rec *rec =
        (struct fixed_posting_list_new_rec *) data;
    PostingPair *pair = rec->fplist->pairs + rec->idx;
    *pair = *((PostingPair *)key);
    // bloom_filter_insert(rec->fplist->filter, ((PostingPair *)key)->doc_id);
    rec->idx++;

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
    fplist->filter = NULL; // bloom_filter_new(NULL, 0.01, fplist->size);

    struct fixed_posting_list_new_rec rec;
    rec.idx = 0;
    rec.fplist = fplist;
    g_tree_foreach(list->list, fixed_posting_list_new_gtree_func, &rec);

    return fplist;
}

gboolean
fixed_posting_list_check_validity(FixedPostingList *list)
{
    // check order
    guint idx;
    PostingPair *pair = list->pairs;
    for(idx = 0; idx < list->size - 1;idx++){
        if (posting_pair_compare_func(pair + idx, pair + idx + 1) >= 0){
            return FALSE;
        }
    }

    return TRUE;
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
    if (!base_list) return NULL;
    if (!succ_list) return NULL;

    guint64 *p1, *p2;
    guint64 *p1_sentinel, *p2_sentinel;
    FixedPostingList *fplist;
    guint size = 0;
    PostingPair *pairs = NULL;
    gint soffset = offset;

    FixedPostingList *inner, *outer;

    inner = base_list;
    outer = succ_list;
    soffset = offset;

    p1 = (guint64 *) inner->pairs;
    p2 = (guint64 *) outer->pairs;
    p1_sentinel = (guint64 *) inner->pairs + base_list->size;
    p2_sentinel = (guint64 *) outer->pairs + succ_list->size;

    while(p1 != p1_sentinel && p2 != p2_sentinel){
        if (*p1 + soffset == *p2){
            size++;
            pairs = g_realloc(pairs, size * sizeof(PostingPair));
            pairs[size - 1] = *(PostingPair *)p1;
            p1++; p2++;
        } else if (*p1 + soffset < *p2) {
            p1++;
        } else {
            p2++;
        }
    }

    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = size;
    fplist->pairs = pairs;
    fplist->filter = NULL;

    return fplist;
}

FixedPostingList *
fixed_posting_list_doc_compact (FixedPostingList *fplist)
{
    if (!fplist) return NULL;
    if (fixed_posting_list_size(fplist) == 0) return fplist;

    FixedPostingList *ret;
    ret = g_malloc(sizeof(FixedPostingList));
    ret->size = 1;
    ret->pairs = g_malloc(sizeof(PostingPair));

    guint doc_id = fplist->pairs[0].doc_id;
    ret->pairs[0].doc_id = doc_id;
    ret->pairs[0].pos = 0;
    PostingPair *pair = fplist->pairs + 1;
    PostingPair *sentiel = fplist->pairs + fplist->size;
    while(pair < sentiel){
        if (doc_id < pair->doc_id) {
            ret->size++;
            ret->pairs = g_realloc(ret->pairs, sizeof(PostingPair) * (ret->size));
            doc_id = ret->pairs[ret->size - 1].doc_id = pair->doc_id;
            ret->pairs[ret->size - 1].pos = 0;
        }
        pair++;
    }

    return ret;
}

FixedPostingList *
fixed_posting_list_doc_intersect (FixedPostingList *fplist1,
                                  FixedPostingList *fplist2)
{
    PostingPair *p1, *p2;
    PostingPair *p1_sentinel, *p2_sentinel;
    FixedPostingList *fplist;
    guint size = 0;
    PostingPair *pairs = NULL;
    guint doc_id = 0;

    if (!fplist1){
        if (!fplist2){
            return NULL;
        } else {
            return fixed_posting_list_doc_compact(fplist2);
        }
    }

    if (!fplist2){
        return fixed_posting_list_doc_compact(fplist1);
    }

    p1 = fplist1->pairs;
    p2 = fplist2->pairs;
    p1_sentinel = fplist1->pairs + fplist1->size;
    p2_sentinel = fplist2->pairs + fplist2->size;

    while(p1 != p1_sentinel && p2 != p2_sentinel){
        if (p1->doc_id == p2->doc_id){
            if (doc_id <= p1->doc_id){
                size++;
                pairs = g_realloc(pairs, size * sizeof(PostingPair));
                pairs[size - 1].doc_id = p1->doc_id;
                pairs[size - 1].pos = 0;
                doc_id = p1->doc_id + 1;
            }
            p1++; p2++;
        } else if (p1->doc_id < p2->doc_id) {
            p1++;
        } else {
            p2++;
        }
    }

    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = size;
    fplist->pairs = pairs;

    return fplist;
}
