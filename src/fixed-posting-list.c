
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
    fplist->skiplist = NULL; // bloom_filter_new(NULL, 0.01, fplist->size);

    struct fixed_posting_list_new_rec rec;
    rec.idx = 0;
    rec.fplist = fplist;
    g_tree_foreach(list->list, fixed_posting_list_new_gtree_func, &rec);

    return fplist;
}

gboolean
fixed_posting_list_check_validity(const FixedPostingList *list)
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
    if(!fplist) return;
    g_free(fplist->pairs);
    g_free(fplist);
}

FixedPostingList *
fixed_posting_list_copy  (const FixedPostingList *fplist)
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
fixed_posting_list_size  (const FixedPostingList *fplist)
{
    return (fplist ? fplist->size : 0);
}

PostingPair *
fixed_posting_list_check (const FixedPostingList *fplist, guint32 doc_id, gint32 pos)
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

static inline guint64 *
move_next(guint64 *p1, guint64 *p1_sentinel, guint64 *p2)
{
    gulong idx1, idx2, tmp;

    if (p1 + 1000 >= p1_sentinel){
        return p1+1;
    }
    if (p1 + 1 == p2){
        return p2;
    }

    idx1 = 0;
    idx2 = 1000;
    while(idx1 != idx2){
        tmp = (idx1 + idx2) >> 1;
        if (p1[tmp] > p1[idx2]){
            idx2 = tmp;
        } else {
            if (tmp == idx1) {
                return p1 + 1;
            } else {
                idx1 = tmp;
            }
        }
    }

    g_printerr("cannot reach here.\n");
}


FixedPostingList *
fixed_posting_list_select_successor (const FixedPostingList *base_list,
                                     const FixedPostingList *succ_list,
                                     guint offset)
{
    if (!base_list) return NULL;
    if (!succ_list) return NULL;

    guint64 *p1, *p2;
    guint64 *p1_sentinel, *p2_sentinel;
    FixedPostingList *fplist;
    guint size = 0;
    guint alloc_size = 1;
    PostingPair *pairs = NULL;
    gint soffset = offset;

    const FixedPostingList *inner, *outer;

    inner = base_list;
    outer = succ_list;
    soffset = offset;

    pairs = g_malloc(alloc_size * sizeof(PostingPair));
    p1 = (guint64 *) inner->pairs;
    p2 = (guint64 *) outer->pairs;
    p1_sentinel = (guint64 *) inner->pairs + base_list->size;
    p2_sentinel = (guint64 *) outer->pairs + succ_list->size;

    while(p1 != p1_sentinel && p2 != p2_sentinel){
        if (*p1 + soffset == *p2){
            size++;
            if (size > alloc_size){
                alloc_size <<= 1;
                pairs = g_realloc(pairs, alloc_size * sizeof(PostingPair));
            }
            pairs[size - 1] = *(PostingPair *)p1;
            p1++; p2++;
        } else if (*p1 + soffset < *p2) {
            // p1 = move_next(p1, p1_sentinel, p2);
            p1++;
        } else {
            // p2 = move_next(p2, p2_sentinel, p1);
            p2++;
        }
    }

    if (size == 0){
        g_free(pairs);
        return NULL;
    }
    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = size;
    fplist->pairs = pairs;
    fplist->skiplist = NULL;

    return fplist;
}


FixedPostingList *
fixed_posting_list_select_successor_with_skiplist (const FixedPostingList *base_list,
                                                   const FixedPostingList *succ_list,
                                                   guint offset)
{
    if (!base_list) return NULL;
    if (!succ_list) return NULL;

    if (!base_list->skiplist || !succ_list->skiplist){
        return fixed_posting_list_select_successor(base_list, succ_list, offset);
    }

    g_return_val_if_reached(NULL);

    return NULL;
}

FixedPostingList *
fixed_posting_list_doc_compact (const FixedPostingList *fplist)
{
    if (!fplist) return NULL;
    if (fixed_posting_list_size(fplist) == 0) return NULL;

    FixedPostingList *ret;
    guint doc_id;
    ret = g_malloc(sizeof(FixedPostingList));
    ret->size = 1;
    ret->pairs = g_malloc(sizeof(PostingPair));

    doc_id = fplist->pairs[0].doc_id;
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
fixed_posting_list_doc_intersect (const FixedPostingList *fplist1,
                                  const FixedPostingList *fplist2)
{
    PostingPair *p1, *p2;
    PostingPair *p1_sentinel, *p2_sentinel;
    FixedPostingList *fplist;
    guint size = 0;
    PostingPair *pairs = NULL;
    guint doc_id = 0;

    // if (!fplist1 |){
    //     if (!fplist2){
    //         return NULL;
    //     } else {
    //         return fixed_posting_list_doc_compact(fplist2);
    //     }
    // }
    // 
    // if (!fplist2){
    //     return fixed_posting_list_doc_compact(fplist1);
    // }

    if (!fplist1) return NULL;
    if (!fplist2) return NULL;

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

Skiplist *
fixed_posting_list_to_skiplist (FixedPostingList *fplist)
{
    Skiplist *sl;
    guint idx;
    guint64 *p;
    gboolean ret;
    sl = skiplist_new();

    for (idx = 0; idx < fplist->size; idx++) {
        p = (gint64 *) &fplist->pairs[idx];
        ret = skiplist_insert(sl, *p);
        g_return_val_if_fail(ret, NULL);
    }

    return sl;
}

FixedPostingList *
fixed_posting_list_from_skiplist_intersect (Skiplist *list1, Skiplist *list2)
{
    FixedPostingList *fplist;
    guint64 *p;
    Skipnode *node1, *node2;
    Skipnode **smaller, **bigger;
    gint level;

    g_return_val_if_fail(list1, NULL);
    g_return_val_if_fail(list2, NULL);

    if (list1->length == 0 || list2->length == 0){
        return NULL;
    }

    fplist = g_malloc(sizeof(FixedPostingList));
    fplist->size = 0;
    fplist->pairs = NULL;
    fplist->skiplist = NULL;

    node1 = list1->head->skips[0];
    node2 = list2->head->skips[0];

    while(node1 != NULL && node2 != NULL){
        if (node1->data == node2->data) {
            fplist->size++;
            fplist->pairs = g_realloc(fplist->pairs, sizeof(PostingPair) * fplist->size);
            p = (gint64 *) &fplist->pairs[fplist->size - 1];
            *p = node1->data;
            node1 = node1->skips[0];
            node2 = node2->skips[0];
        } else if (node1->data < node2->data) {
            if (node1->skips[0] == NULL) break;
            for (level = node1->level; level >= 0; level--) {
                if (node1->skips[level]->data <= node2->data) {
                    node1 = node1->skips[level];
                    break;
                }
            }
        } else { // if (node1->data < node2->data)
            if (node2->skips[0] == NULL) break;
            for (level = node2->level; level >= 0; level--) {
                if (node2->skips[level]->data <= node1->data) {
                    node2 = node2->skips[level];
                    break;
                }
            }
        }
    }

    return fplist;
}
