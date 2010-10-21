
#include "skiplist.h"

static gdouble level_probs[] = {1.0, 0.5, 0.5*0.5, 0.5*0.5*0.5, 0.5*0.5*0.5*0.5};

Skiplist *
skiplist_new       (void)
{
    Skiplist *list;
    Skipnode *head;
    gdouble r;
    gint level;
    list = g_slice_new(Skiplist);
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;

    // dummy header node
    head = g_slice_new(Skipnode);
    head->level = SKIPLIST_MAX_LEVEL - 1;
    head->data = 0;
    head->skips = list->skips;
    list->head = head;

    for (level = 0; level < SKIPLIST_MAX_LEVEL; level++){
        list->skips[level] = NULL;
    }

    return list;
}

void
skiplist_free (Skiplist *list)
{
    Skipnode *tmp1, *tmp2;
    for (tmp1 = list->head; tmp1 != NULL; tmp1 = tmp2){
        tmp2 = tmp1->skips[0];
        if (tmp1 != list->head)
            g_free(tmp1->skips);
        g_slice_free(Skipnode, tmp1);
    }
    g_slice_free(Skiplist, list);
}

guint
skiplist_length    (Skiplist *list)
{
    return list->length;
}

static
Skipnode *
skipnode_new (guint64 data)
{
    Skipnode *node;
    gdouble r;
    guint level;
    node = g_slice_new(Skipnode);
    node->data = data;

    r = drand48();
    for (level = 0; level < SKIPLIST_MAX_LEVEL; level++){
        if (r > level_probs[level])
            break;
        node->level = level;
    }
    node->skips = g_malloc(sizeof(Skipnode) * (level + 1));
    bzero(node->skips, sizeof(Skipnode) * (level + 1));

    return node;
}

gboolean
skiplist_insert    (Skiplist *list, guint64 data)
{
    Skipnode *pred;
    Skipnode *new;
    Skipnode *new_skips[SKIPLIST_MAX_LEVEL];
    gint level;

    g_return_val_if_fail(list, FALSE);

    if (skiplist_exists(list, data) == TRUE){
        return FALSE;
    }

    new = skipnode_new(data);
    pred = list->head;
    for (level = new->level; level >= 0; level--){
        while (pred != NULL) {
            if (pred->skips[level] == NULL) {
                pred->skips[level] = new;
                break;
            } else if (data > pred->skips[level]->data) {
                pred = pred->skips[level];
            } else if (data < pred->skips[level]->data) {
                new->skips[level] = pred->skips[level];
                pred->skips[level] = new;
                break;
            } else {
                // impossible case.
                g_return_val_if_reached(FALSE);
            }
        }
    }
    list->length++;
    return TRUE;
}

gboolean
skiplist_exists (Skiplist *list, guint64 data)
{
    gint level;
    Skipnode *pred;

    if (list->length == 0) return FALSE;

    pred = list->head;
    for (level = SKIPLIST_MAX_LEVEL - 1; level >= 0; level--){
        while (pred != NULL && pred->skips[level] != NULL) {
            if (data > pred->skips[level]->data) {
                pred = pred->skips[level];
            } else if (data == pred->skips[level]->data) {
                return TRUE;
            } else { // if data < pred->skips[level]->data
                break;
            }
        }
    }

    return FALSE;
}
