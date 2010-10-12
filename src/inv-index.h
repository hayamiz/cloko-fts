#ifndef INV_INDEX_H
#define INV_INDEX_H

#include <glib.h>

typedef struct _PostingPair {
    gint doc_id;
    gint pos;
} PostingPair;

typedef struct _PostingList {
    GSList *list;
} PostingList;

typedef struct _InvIndex {
    GHashTable *hash;
} InvIndex;


gint posting_pair_compare_func (gconstpointer a, gconstpointer b);

PostingList *posting_list_new       (void);
guint        posting_list_size      (PostingList *posting_list);
void         posting_list_add       (PostingList *posting_list, gint doc_id, gint pos);
GSList      *posting_list_get_slist (PostingList *posting_list);

InvIndex    *inv_index_new       (void);
int          inv_index_numterms  (InvIndex *inv_index);
PostingList *inv_index_get       (InvIndex *inv_index, const gchar *term);
void         inv_index_add_term  (InvIndex *inv_index, const gchar *term, gint doc_id, gint pos);

#endif
