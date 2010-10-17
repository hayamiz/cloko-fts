
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
    bloom_filter_insert(rec->fplist->filter, ((PostingPair *)key)->doc_id);
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
    fplist->filter = bloom_filter_new(NULL, 0.01, fplist->size);

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

static inline PostingPair *
next_doc (PostingPair *pair, PostingPair *sentinel)
{
    guint cur_doc_id;
    cur_doc_id = pair->doc_id;

    if (pair >= sentinel){
        return NULL;
    }

    if (pair + 1 < sentinel && cur_doc_id == (pair + 1)->doc_id){
        return pair + 1;
    }

    int idx1, idx2, tmp_idx;
    idx1 = 0;
    idx2 = (sentinel - pair) - 1;
    if (cur_doc_id == pair[idx2].doc_id){
        return sentinel;
    }

    for(;;){
        tmp_idx = (idx1 + idx2) >> 1;
        if (cur_doc_id < pair[tmp_idx].doc_id){
            idx2 = tmp_idx;
        } else if (cur_doc_id == pair[tmp_idx].doc_id){
            if (idx1 == tmp_idx){
                return pair + idx2;
            } else {
                idx1 = tmp_idx;
            }
        }
    }
}
FixedPostingList *
fixed_posting_list_select_successor (FixedPostingList *base_list,
                                     FixedPostingList *succ_list,
                                     guint offset)
{
    if (!base_list) return NULL;
    if (!succ_list) return NULL;

    FixedPostingList *inner, *outer;
    PostingPair *p1, *p2, **bp;
    PostingPair *p1_sentinel, *p2_sentinel;
    gint p1doc_id, p2doc_id;
    FixedPostingList *fplist;
    gint soffset;
    guint size = 0;
    PostingPair *pairs = NULL;

    if (fixed_posting_list_size(base_list) <=
        fixed_posting_list_size(succ_list)){
        soffset = offset;
        inner = base_list;
        outer = succ_list;
        bp = &p1;
    } else {
        soffset = -1 * offset;
        inner = succ_list;
        outer = base_list;
        bp = &p2;
    }

    p1 = inner->pairs;
    p2 = outer->pairs;
    p1_sentinel = p1 + inner->size;
    p2_sentinel = p2 + outer->size;

    while(p1 != p1_sentinel && p2 != p2_sentinel){
        if (p1->doc_id > p2->doc_id) {
            p2 = next_doc(p2, p2_sentinel);
        } else if (p1->doc_id < p2->doc_id) {
            p1 = next_doc(p1, p1_sentinel);
        } else {
            if (outer->filter != NULL &&
                bloom_filter_check(outer->filter, p1->doc_id) == FALSE){
                p1 = next_doc(p1, p1_sentinel);
                p2 = next_doc(p2, p2_sentinel);
            } else {
                p1doc_id = p1->doc_id;
                p2doc_id = p2->doc_id;
                do {
                    p1->pos += soffset;
                    if (posting_pair_compare_func(p1, p2) == 0){
                        p1->pos -= soffset; // restore
                        size++;
                        pairs = g_realloc(pairs, size * sizeof(PostingPair));
                        pairs[size - 1] = **bp;
                        p1++; p2++;
                    } else if (posting_pair_compare_func(p1, p2) < 0) {
                        p1->pos -= soffset; // restore
                        p1++;
                    } else {
                        p1->pos -= soffset; // restore
                        p2++;
                    }
                } while (p1 != p1_sentinel &&
                         p2 != p2_sentinel &&
                         p1->doc_id == p1doc_id &&
                         p2->doc_id == p2doc_id);
            }
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
