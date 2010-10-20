#ifndef INV_INDEX_H
#define INV_INDEX_H

#include <string.h>
#include <glib.h>
#include <mecab.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <limits.h>

#include <bloom-filter.h>
#include <thread-pool.h>

struct _DocumentSet;

typedef struct _Document {
    gint  id;
    guint pos;
    guint time;
    guint size;
    guint body_size;
    gchar *title;
    gchar *url;
    gchar *url_top;
    guint body_pointer_offset;
    const struct _DocumentSet *docset;
} Document;

typedef struct _DocumentSet {
    gchar *buffer;
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
void         document_free         (Document *doc);
gint         document_id           (Document *doc);
guint        document_position     (Document *doc);
const gchar *document_body_pointer (Document *doc);
guint        document_body_size    (Document *doc);
const gchar *document_title        (Document *doc);
const gchar *document_url          (Document *doc);
guint        document_time         (Document *doc);
gchar       *document_raw_record   (Document *doc);

DocumentSet *document_set_load   (const gchar *path, guint limit);
DocumentSet *document_set_free   (DocumentSet *docset);
guint        document_set_size   (DocumentSet *docset);
Document    *document_set_nth    (DocumentSet *docset, guint idx);
const gchar *document_set_buffer (DocumentSet *docset);
FixedIndex  *document_set_make_fixed_index (DocumentSet *docset, guint limit);
void         document_set_indexing_show_progress (gboolean sw);

Tokenizer   *tokenizer_new  (const gchar *str);
Tokenizer   *tokenizer_new2 (const gchar *str, guint len);
Tokenizer   *tokenizer_renew  (Tokenizer *tok, const gchar *str);
Tokenizer   *tokenizer_renew2 (Tokenizer *tok, const gchar *str, guint len);
void         tokenizer_free (Tokenizer *tok);
gchar       *tokenizer_next (Tokenizer *tok);

Phrase      *phrase_new         (void);
Phrase      *phrase_from_string (const gchar *str);
void         phrase_free        (Phrase *phrase);
guint        phrase_size        (Phrase *phrase);
void         phrase_append      (Phrase *phrase, const gchar *term);
const gchar *phrase_nth         (Phrase *phrase, guint idx);

gint posting_pair_compare_func (gconstpointer a, gconstpointer b);
PostingPair *posting_pair_new  (guint32 doc_id, gint32 pos);
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
gboolean          fixed_posting_list_check_validity (const FixedPostingList *list);
FixedPostingList *fixed_posting_list_copy  (const FixedPostingList *fixed_posting_list);
guint             fixed_posting_list_size  (const FixedPostingList *fixed_posting_list);
PostingPair      *fixed_posting_list_check (const FixedPostingList *fixed_posting_list, guint32 doc_id, gint32 pos);
FixedPostingList *fixed_posting_list_select_successor (const FixedPostingList *base_list,
                                                       const FixedPostingList *successor_list,
                                                       guint offset);
FixedPostingList *fixed_posting_list_doc_compact (const FixedPostingList *fplist1);
FixedPostingList *fixed_posting_list_doc_intersect (const FixedPostingList *fplist1,
                                                    const FixedPostingList *fplist2);

FixedIndex       *fixed_index_new        (InvIndex *inv_index);
void              fixed_index_free       (FixedIndex *fixed_index);
gboolean          fixed_index_check_validity (const FixedIndex *findex);
int               fixed_index_numterms   (const FixedIndex *fixed_index);
FixedPostingList *fixed_index_get        (const FixedIndex *fixed_index, const gchar *term);
FixedPostingList *fixed_index_phrase_get (const FixedIndex *fixed_index, Phrase *phrase);
const FixedIndex *fixed_index_dump       (const FixedIndex *fixed_index, const gchar *path);
FixedIndex       *fixed_index_load       (const gchar *path);
FixedPostingList *fixed_index_multiphrase_get (const FixedIndex *findex, GList *phrases);
ThreadPool       *fixed_index_make_thread_pool (guint thread_num);
FixedPostingList *fixed_index_multithreaded_multiphrase_get (const FixedIndex *findex,
                                                             ThreadPool *pool,
                                                             GList *list);

#define FRAME_HEADER_SIZE (3 * sizeof(guint32))
#define FRAME_SIZE        8192
#define FRAME_BODY_SIZE   (FRAME_SIZE - FRAME_HEADER_SIZE)
#define FRAME_CONTENT_SIZE   (FRAME_SIZE - FRAME_HEADER_SIZE - sizeof(guint32))

typedef enum {
    FRM_QUERY = 0,
    FRM_LONG_QUERY_FIRST = 1,
    FRM_LONG_QUERY_REST = 2,
    FRM_RESULT = 3,
    FRM_LONG_RESULT_FIRST = 4,
    FRM_LONG_RESULT_REST = 5,
    FRM_BYE = 6,
    FRM_QUIT = 7,
} FrameType;

typedef struct _FrameQueryBody {
    guint32 length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameQueryBody;

typedef struct _FrameLongQueryFirstBody {
    guint32 frm_length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameLongQueryFirstBody;

typedef struct _FrameLongQueryRestBody {
    guint32 length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameLongQueryRestBody;

typedef struct _FrameResultBody {
    guint32 length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameResultBody;

typedef struct _FrameLongResultFisrstBody {
    guint32 frm_length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameLongResultFirstBody;

typedef struct _FrameLongResultRestBody {
    guint32 length;
    gchar buf[FRAME_CONTENT_SIZE];
} FrameLongResultRestBody;

typedef struct _Frame {
    guint32 type;
    guint32 content_length;
    guint32 extra_field;
    union {
        FrameQueryBody q;
        FrameLongQueryFirstBody lqf;
        FrameLongQueryRestBody  lqr;
        FrameResultBody r;
        FrameLongResultFirstBody lrf;
        FrameLongResultRestBody lrr;
    } body;
} Frame;

typedef union _RawFrame {
    gchar buf[FRAME_SIZE];
    Frame frame;
} RawFrame;

Frame   *frame_new               (void);
void    frame_free               (Frame *frame);
void    frame_make_quit          (Frame *frame);
GList   *frame_make_result_from_fixed_posting_list (DocumentSet *docset,
                                                  FixedPostingList *fplist);
gssize  frame_send               (gint sockfd, Frame *frame);
gssize  frame_send_multi_results (gint sockfd, GList *frames);
gssize  frame_send_query         (gint sockfd, const gchar *query);
gssize  frame_send_bye           (gint sockfd);
gssize  frame_send_quit          (gint sockfd);
gssize  frame_recv               (gint sockfd, Frame *frame);
GList  *frame_recv_result        (gint sockfd);
guint32 frame_type               (Frame *frame);

#endif
