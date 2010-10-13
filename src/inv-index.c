#include "inv-index.h"


Phrase *
phrase_new (void)
{
    Phrase *phrase = g_malloc(sizeof(Phrase));
    phrase->size = 0;
    phrase->terms = NULL;

    return phrase;
}

guint
phrase_size (Phrase *phrase)
{
    return phrase->size;
}

void
phrase_append (Phrase *phrase, const gchar *term)
{
    phrase->size ++;
    phrase->terms = g_realloc(phrase->terms, phrase->size * sizeof(const gchar *));
    phrase->terms[phrase->size - 1] = term;
}

const gchar *
phrase_nth (Phrase *phrase, guint idx)
{
    if (idx >= phrase_size(phrase)){
        return NULL;
    }

    return phrase->terms[idx];
}

InvIndex *
inv_index_new (void)
{
    InvIndex *inv_index = g_malloc(sizeof(InvIndex));

    inv_index->hash = g_hash_table_new(g_str_hash, g_str_equal);

    return inv_index;
}

int
inv_index_numterms (InvIndex *inv_index)
{
    return g_hash_table_size(inv_index->hash);
}

void
inv_index_add_term (InvIndex *inv_index, const gchar *term, gint doc_id, gint pos)
{
    PostingList *posting_list;
    GHashTable *hash_table = inv_index->hash;

    if ((posting_list = g_hash_table_lookup(hash_table, term)) == NULL){
        posting_list = posting_list_new();
        g_hash_table_insert(hash_table, (gpointer) term, posting_list);
    }

    posting_list_add(posting_list, doc_id, pos);
}

PostingList *
inv_index_get (InvIndex *inv_index, const gchar *term)
{
    return (PostingList *) g_hash_table_lookup(inv_index->hash, term);
}

PostingList *
inv_index_phrase_get (InvIndex *inv_index, Phrase *phrase)
{
    if (inv_index == NULL ||
        phrase == NULL ||
        phrase_size(phrase) == 0){
        return NULL;
    } else if (phrase_size(phrase) == 1){
        return inv_index_get(inv_index, phrase_nth(phrase, 0));
    }

    PostingList *base_list = inv_index_get(inv_index, phrase_nth(phrase, 0));
    PostingList *succ_list;
    guint phrase_idx;
    guint sz = phrase_size(phrase);
    if (base_list == NULL) {
        return NULL;
    } else {
        base_list = posting_list_copy(base_list);
    }

    for(phrase_idx = 1;phrase_idx < sz && posting_list_size(base_list) > 0;phrase_idx++){
        succ_list = inv_index_get(inv_index, phrase_nth(phrase, phrase_idx));
        base_list = posting_list_select_successor(base_list, succ_list, phrase_idx);
    }

    if (posting_list_size(base_list) == 0){
        return NULL;
    }

    return base_list;
}
