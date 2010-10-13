
#include "inv-index.h"

static void
fixed_index_new_ghash_func(gpointer key, gpointer val, gpointer user_data)
{
    GHashTable *table = (GHashTable *) user_data;
    g_hash_table_insert(table, key, fixed_posting_list_new((PostingList *) val));
}

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
}

void
fixed_index_free       (FixedIndex *findex)
{
    if (!findex) return;

    g_hash_table_foreach(findex->hash, fixed_index_free_ghash_func, NULL);
    g_hash_table_unref(findex->hash);
    g_free(findex);
}

int
fixed_index_numterms   (FixedIndex *findex)
{
    if (!findex) return 0;
    return g_hash_table_size(findex->hash);
}

FixedPostingList *
fixed_index_get        (FixedIndex *findex, const gchar *term)
{
    if (!findex) return NULL;

    return g_hash_table_lookup(findex->hash, term);
}

FixedPostingList *
fixed_index_phrase_get (FixedIndex *findex, Phrase *phrase)
{
    if (!findex || !phrase || phrase_size(phrase) == 0)
        return NULL;
    else if (phrase_size(phrase) == 1)
        return fixed_index_get(findex, phrase_nth(phrase, 0));

    FixedPostingList *base_list = fixed_index_get(findex, phrase_nth(phrase, 0));
    FixedPostingList *succ_list;
    guint phrase_idx;
    guint sz = phrase_size(phrase);
    if (base_list == NULL) {
        return NULL;
    }

    for(phrase_idx = 1; phrase_idx < sz && fixed_posting_list_size(base_list); phrase_idx++){
        succ_list = fixed_index_get(findex, phrase_nth(phrase, phrase_idx));
        FixedPostingList *tmp;
        tmp = fixed_posting_list_select_successor(base_list, succ_list, phrase_idx);
        if (phrase_idx > 1) fixed_posting_list_free(base_list);
        base_list = tmp;
    }

    if (fixed_posting_list_size(base_list) == 0){
        return NULL;
    }

    return base_list;
}

void
fixed_index_dump       (const gchar *path, FixedIndex *findex)
{
    // TODO
    return;
}

FixedIndex *
fixed_index_load       (const gchar *path)
{
    // TODO
    return NULL;
}
