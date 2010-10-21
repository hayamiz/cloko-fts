
#include "inv-index.h"

static void
fixed_index_new_ghash_func(gpointer key, gpointer val, gpointer user_data)
{
    GHashTable *table = (GHashTable *) user_data;
    FixedPostingList *fplist;
    fplist = fixed_posting_list_new((PostingList *) val);
    fplist->skiplist = fixed_posting_list_to_skiplist(fplist);
    g_hash_table_insert(table,
                        g_strdup((const gchar *) key),
                        fplist);
}

/* term (key in hash table) is strdup-ed. */
FixedIndex *
fixed_index_new        (InvIndex *inv_index)
{
    if (!inv_index) return NULL;
    FixedIndex *findex = g_malloc(sizeof(FixedIndex));
    findex->hash = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_foreach(inv_index->hash, fixed_index_new_ghash_func, findex->hash);

    return findex;
}

static void
fixed_index_free_ghash_func(gpointer key, gpointer val, gpointer user_data)
{
    fixed_posting_list_free((FixedPostingList *)val);
    g_free(key);
}

void
fixed_index_free       (FixedIndex *findex)
{
    if (!findex) return;

    g_hash_table_foreach(findex->hash, fixed_index_free_ghash_func, NULL);
    g_hash_table_unref(findex->hash);
    g_free(findex);
}

static void
fixed_index_check_validity_ghash_func(gpointer key,
                                      gpointer value,
                                      gpointer user_data)
{
    gboolean *ret;
    const gchar *term;
    term = (const gchar *) key;
    ret = (gboolean *) user_data;
    if (fixed_posting_list_check_validity((FixedPostingList *) value) == FALSE){
        *ret = FALSE;
        fprintf(stderr, "broken index for the term \"%s\"\n", term);
    }
}
gboolean
fixed_index_check_validity (const FixedIndex *findex)
{
    gboolean ret;
    ret = TRUE;

    g_hash_table_foreach(findex->hash,
                         fixed_index_check_validity_ghash_func,
                         &ret);

    return ret;
}

int
fixed_index_numterms   (const FixedIndex *findex)
{
    if (!findex) return 0;
    return g_hash_table_size(findex->hash);
}

FixedPostingList *
fixed_index_get        (const FixedIndex *findex, const gchar *term)
{
    FixedPostingList *fplist;

    if (!findex) return NULL;

    fplist = g_hash_table_lookup(findex->hash, term);

    if (!fplist)
        return NULL;

    return fixed_posting_list_copy(fplist);
}

FixedPostingList *
fixed_index_phrase_get (const FixedIndex *findex, Phrase *phrase)
{
    FixedPostingList *base_list;
    FixedPostingList *succ_list;
    FixedPostingList *tmp;
    guint phrase_idx;
    guint sz;

    if (!findex || !phrase || phrase_size(phrase) == 0)
        return NULL;
    else if (phrase_size(phrase) == 1)
        return fixed_index_get(findex, phrase_nth(phrase, 0));

    base_list = fixed_index_get(findex, phrase_nth(phrase, 0));
    if (base_list == NULL) {
        return NULL;
    }
    succ_list = NULL;
    tmp = NULL;
    sz = phrase_size(phrase);

    for(phrase_idx = 1; phrase_idx < sz && fixed_posting_list_size(base_list); phrase_idx++){
        succ_list = fixed_index_get(findex, phrase_nth(phrase, phrase_idx));
        tmp = fixed_posting_list_select_successor(base_list, succ_list, phrase_idx);
        fixed_posting_list_free(base_list);
        fixed_posting_list_free(succ_list);
        base_list = tmp;
    }

    if (fixed_posting_list_size(base_list) == 0){
        fixed_posting_list_free(base_list);
        return NULL;
    }

    return base_list;
}

static void
dump_ghash_func(gpointer key, gpointer val, gpointer data)
{
    FILE *fp = (FILE *)data;
    const gchar *term = (const gchar *) key;
    FixedPostingList *fplist = (FixedPostingList *) val;

    guint32 termlen = strlen(term) + 1;
    guint32 fplistlen = fixed_posting_list_size(fplist);
    fwrite(&termlen, sizeof(guint32), 1, fp);
    fwrite(term, sizeof(gchar), termlen, fp);

    fwrite(&fplistlen, sizeof(guint32), 1, fp);
    fwrite(fplist->pairs, sizeof(PostingPair), fplistlen, fp);
}

const FixedIndex *
fixed_index_dump       (const FixedIndex *findex, const gchar *path)
{
    FILE *fp;

    if ((fp = fopen(path, "w")) == NULL){
        return NULL;
    }
    guint32 numterms = fixed_index_numterms(findex);
    fwrite(&numterms, sizeof(guint32), 1, fp);
    g_hash_table_foreach(findex->hash, dump_ghash_func, fp);
    fclose(fp);

    return findex;
}

FixedIndex *
fixed_index_load       (const gchar *path)
{
    FILE *fp;
    guint32 numterms;
    guint32 termlen;
    guint32 fplistlen;
    gchar *term;
    size_t sz;
    FixedIndex *findex;
    FixedPostingList *fplist;

    findex = g_malloc(sizeof(FixedIndex));
    findex->hash = g_hash_table_new(g_str_hash, g_str_equal);

    if ((fp = fopen(path, "r")) == NULL){
        return NULL;
    }
    sz = fread(&numterms, sizeof(guint32), 1, fp); if (sz != 1) return NULL;
    for(;numterms > 0;numterms--){
        sz = fread(&termlen, sizeof(guint32), 1, fp); if (sz != 1) return NULL;
        term = g_malloc(sizeof(gchar) * termlen);
        sz = fread(term, sizeof(gchar), termlen, fp); if (sz != termlen) return NULL;

        sz = fread(&fplistlen, sizeof(guint32), 1, fp); if (sz != 1) return NULL;
        fplist = g_malloc(sizeof(FixedPostingList));
        fplist->size = fplistlen;
        fplist->pairs = g_malloc(sizeof(PostingPair) * fplistlen);
        sz = fread(fplist->pairs, sizeof(PostingPair), fplistlen, fp); if (sz != fplistlen) return NULL;

        g_hash_table_insert(findex->hash, term, fplist);
    }

    fclose(fp);
    return findex;
}

FixedPostingList *
fixed_index_multiphrase_get (const FixedIndex *findex, GList *phrases)
{
    FixedPostingList *base_list, *succ_list, *tmp_list;
    GList *phrase;

    if (g_list_length(phrases) == 0){
        return NULL;
    }

    phrase = phrases;
    base_list = fixed_index_phrase_get(findex, phrase->data);
    phrase = phrase->next;

    for(; base_list != NULL && phrase != NULL; phrase = phrase->next) {
        succ_list = fixed_index_phrase_get(findex, phrase->data);
        tmp_list = fixed_posting_list_doc_intersect(base_list, succ_list);
        fixed_posting_list_free(base_list);
        fixed_posting_list_free(succ_list);
        base_list = tmp_list;
    }

    return base_list;
}

typedef struct _tp_arg {
    const FixedIndex *findex;
    Phrase *phrase;
    FixedPostingList *list;
} tp_arg_t;

static void
fixed_index_multithreaded_multiphrase_get_thread_func (GAsyncQueue *output, gpointer task)
{
    tp_arg_t *arg;

    arg = task;
    arg->list = fixed_index_phrase_get(arg->findex, arg->phrase);

    g_async_queue_push(output, arg);
}

ThreadPool *
fixed_index_make_thread_pool (guint thread_num)
{
    return thread_pool_new(thread_num,
                           fixed_index_multithreaded_multiphrase_get_thread_func);
}

FixedPostingList *
fixed_index_multithreaded_multiphrase_get (const FixedIndex *findex,
                                           ThreadPool *pool,
                                           GList *phrases)
{
    FixedPostingList *base_list, *succ_list, *tmp_list;
    tp_arg_t *arg;
    GList *phrase_cell;
    guint idx;

    g_return_val_if_fail(findex, NULL);
    g_return_val_if_fail(pool, NULL);
    g_return_val_if_fail(phrases, NULL);

    for(phrase_cell = phrases; phrase_cell != NULL; phrase_cell = phrase_cell->next) {
        arg = g_malloc(sizeof(tp_arg_t));
        arg->findex = findex;
        arg->phrase = phrase_cell->data;
        arg->list = NULL;
        thread_pool_push(pool, arg);
    }

    arg = thread_pool_pop(pool);
    base_list = arg->list;
    g_free(arg);

    for (idx = 1; idx < g_list_length(phrases); idx++) {
        arg = thread_pool_pop(pool);
        succ_list = arg->list;
        tmp_list = fixed_posting_list_doc_intersect(base_list, succ_list);
        fixed_posting_list_free(base_list);
        fixed_posting_list_free(succ_list);
        g_free(arg);
        base_list = tmp_list;
    }

    return base_list;
}
