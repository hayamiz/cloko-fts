#ifndef INV_INDEX_H
#define INV_INDEX_H

#include <string.h>
#include <glib.h>
#include <mecab.h>
#include <bloom-filter.h>

struct _DocumentSet;

typedef struct _Document {
    gint  id;
    guint pos;
    guint time;
    guint size;
    guint body_size;
    const gchar *title;
    const gchar *url;
    const gchar *url_top;
    const gchar *body_pointer;
    struct _DocumentSet *docset;
} Document;

typedef struct _DocumentSet {
    const gchar *buffer;
    guint buffer_size;
    Document **docs;
    guint size;
} DocumentSet;

typedef struct _Tokenizer {
    mecab_t *mecab;
    const mecab_node_t *cur_node;
} Tokenizer;

typedef struct _Phrase {
    guint size;
    gchar **terms;
} Phrase;

// PAY ATTENTION FOR ENDIAN
typedef struct _PostingPair {
    gint32 pos;
    guint32 doc_id;
} PostingPair;

typedef struct _PostingList {
    GTree *list;
} PostingList;

typedef struct _InvIndex {
    GHashTable *hash;
} InvIndex;

typedef struct _FixedPostingList {
    guint size;
    PostingPair *pairs;
    BloomFilter *filter;
} FixedPostingList;

typedef struct _FixedIndex {
    GHashTable *hash;
} FixedIndex;

Document    *document_parse        (DocumentSet  *docset,
                                    const gchar  *text,
                                    guint         offset,
                                    gint          doc_id,
                                    const gchar **endptr);
gint         document_id           (Document *doc);
guint        document_position     (Document *doc);
const gchar *document_body_pointer (Document *doc);
guint        document_body_size    (Document *doc);
const gchar *document_title        (Document *doc);
const gchar *document_url          (Document *doc);
guint        document_time         (Document *doc);
gchar       *document_raw_record   (Document *doc);

DocumentSet *document_set_load   (const gchar *path, guint limit);
guint        document_set_size   (DocumentSet *docset);
Document    *document_set_nth    (DocumentSet *docset, guint idx);
const gchar *document_set_buffer (DocumentSet *docset);

Tokenizer   *tokenizer_new  (const gchar *str);
Tokenizer   *tokenizer_new2 (const gchar *str, guint len);
Tokenizer   *tokenizer_renew  (Tokenizer *tok, const gchar *str);
Tokenizer   *tokenizer_renew2 (Tokenizer *tok, const gchar *str, guint len);
void         tokenizer_free (Tokenizer *tok);
const gchar *tokenizer_next (Tokenizer *tok);

Phrase      *phrase_new    (void);
void         phrase_free   (Phrase *phrase);
guint        phrase_size   (Phrase *phrase);
void         phrase_append (Phrase *phrase, const gchar *term);
const gchar *phrase_nth    (Phrase *phrase, guint idx);

gint posting_pair_compare_func (gconstpointer a, gconstpointer b);
PostingPair *posting_pair_new (guint32 doc_id, gint32 pos);
PostingPair *posting_pair_free (PostingPair *pair);

PostingList *posting_list_new   (void);
void         posting_list_free  (PostingList *posting_list, gboolean free_pairs);
PostingList *posting_list_copy  (PostingList *posting_list);
guint        posting_list_size  (PostingList *posting_list);
void         posting_list_add   (PostingList *posting_list, guint32 doc_id, gint32 pos);
PostingPair *posting_list_check (PostingList *posting_list, guint32 doc_id, gint32 pos);
PostingList *posting_list_select_successor (PostingList *base_list,
                                            PostingList *successor_list,
                                            guint offset);

InvIndex    *inv_index_new        (void);
void         inv_index_free       (InvIndex *inv_index);
int          inv_index_numterms   (InvIndex *inv_index);
PostingList *inv_index_get        (InvIndex *inv_index, const gchar *term);
void         inv_index_add_term   (InvIndex *inv_index, const gchar *term, gint doc_id, gint pos);
PostingList *inv_index_phrase_get (InvIndex *inv_index, Phrase *phrase);


FixedPostingList *fixed_posting_list_new   (PostingList *list);
void              fixed_posting_list_free  (FixedPostingList *posting_list);
gboolean          fixed_posting_list_check_validity (FixedPostingList *list);
FixedPostingList *fixed_posting_list_copy  (FixedPostingList *fixed_posting_list);
guint             fixed_posting_list_size  (FixedPostingList *fixed_posting_list);
PostingPair      *fixed_posting_list_check (FixedPostingList *fixed_posting_list, guint32 doc_id, gint32 pos);
FixedPostingList *fixed_posting_list_select_successor (FixedPostingList *base_list,
                                                       FixedPostingList *successor_list,
                                                       guint offset);
FixedPostingList *fixed_posting_list_doc_compact (FixedPostingList *fplist1);
FixedPostingList *fixed_posting_list_doc_intersect (FixedPostingList *fplist1,
                                                    FixedPostingList *fplist2);

FixedIndex       *fixed_index_new        (InvIndex *inv_index);
void              fixed_index_free       (FixedIndex *fixed_index);
gboolean          fixed_index_check_validity (FixedIndex *findex);
int               fixed_index_numterms   (FixedIndex *fixed_index);
FixedPostingList *fixed_index_get        (FixedIndex *fixed_index, const gchar *term);
FixedPostingList *fixed_index_phrase_get (FixedIndex *fixed_index, Phrase *phrase);
FixedIndex       *fixed_index_dump       (FixedIndex *fixed_index, const gchar *path);
FixedIndex       *fixed_index_load       (const gchar *path);

#endif
