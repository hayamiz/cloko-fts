#ifndef INV_INDEX_H
#define INV_INDEX_H

#include <glib.h>

typedef struct _Phrase {
    guint size;
    const gchar **terms;
} Phrase;

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

Phrase      *phrase_new(void);
guint        phrase_size(Phrase *phrase);
void         phrase_append(Phrase *phrase, const gchar *term);
const gchar *phrase_nth(Phrase *phrase, guint idx);

gint posting_pair_compare_func (gconstpointer a, gconstpointer b);

PostingList *posting_list_new               (void);
PostingList *posting_list_copy              (PostingList *posting_list);
guint        posting_list_size              (PostingList *posting_list);
void         posting_list_add               (PostingList *posting_list, gint doc_id, gint pos);
PostingPair *posting_list_nth               (PostingList *posting_list, guint idx);
PostingList *posting_list_select_successor (PostingList *base_list,
                                            PostingList *successor_list,
                                            guint offset);

InvIndex    *inv_index_new        (void);
int          inv_index_numterms   (InvIndex *inv_index);
PostingList *inv_index_get        (InvIndex *inv_index, const gchar *term);
void         inv_index_add_term   (InvIndex *inv_index, const gchar *term, gint doc_id, gint pos);
PostingList *inv_index_phrase_get (InvIndex *inv_index, Phrase *phrase);

#endif
