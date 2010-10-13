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

PostingList *
posting_list_copy (PostingList *orig_list)
{
    PostingList *posting_list;
    posting_list = g_malloc(sizeof(PostingList));
    posting_list->list = g_slist_copy(orig_list->list);

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
        return;
    }

    GSList *list = posting_list->list;
    GSList *pred_list = posting_list->list;

    gint cmp;

    cmp = posting_pair_compare_func(pair, (PostingPair *) list->data);
    if (cmp < 0){
        posting_list->list = g_slist_prepend(list, pair);
        return;
    } else if (cmp == 0) {
        return;
    }

    list = list->next;

    while(list != NULL){
        cmp = posting_pair_compare_func(pair, (PostingPair *) list->data);
        if (cmp == 0){
            return;
        } else if (cmp < 0){
            pred_list = g_slist_insert_before(pred_list, list, pair);
            return;
        }

        pred_list = list;
        list = list->next;
    }

    pred_list = g_slist_append(pred_list, pair);
}

PostingPair *
posting_list_nth (PostingList *posting_list, guint idx)
{
    if (posting_list == NULL ||
        idx >= g_slist_length(posting_list->list)) {
        return NULL;
    }

    return (PostingPair *) g_slist_nth_data(posting_list->list, idx);
}

PostingList *
posting_list_select_successor (PostingList *base_list,
                               PostingList *successor_list,
                               guint offset)
{
    if (posting_list_size(base_list) == 0 ||
        posting_list_size(successor_list) == 0)
    {
        return NULL;
    }

    GSList *list = base_list->list;
    GSList *pred_list = list;
    GSList *succ_list = successor_list->list;
    PostingPair *base_pair;
    PostingPair *succ_pair;

    while(list != NULL){
        base_pair = g_slist_nth_data(list, 0);

        while(succ_list != NULL){
            succ_pair = g_slist_nth_data(succ_list, 0);
            if (succ_pair->doc_id > base_pair->doc_id){
                break;
            } else if (succ_pair->doc_id == base_pair->doc_id){
                if (succ_pair->pos == base_pair->pos + offset){
                    succ_list = g_slist_next(succ_list);
                    goto selected;
                }
            }
            succ_list = g_slist_next(succ_list);
        }

        // no matching successor
        if (pred_list == list) { // head of the list
            base_list->list = g_slist_delete_link(base_list->list, list);
            list = pred_list = base_list->list;
        } else {
            pred_list = g_slist_delete_link(pred_list, list);
            list = g_slist_next(pred_list);
        }
        continue;

    selected:
        pred_list = list;
        list = g_slist_next(list);
    }

    return base_list;
}

