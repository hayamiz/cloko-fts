
#include "inv-index.h"


static inline void phrase_append_nocopy (Phrase *phrase, gchar *term);

Phrase *
phrase_new (void)
{
    Phrase *phrase = g_malloc(sizeof(Phrase));
    phrase->size = 0;
    phrase->terms = NULL;

    return phrase;
}

Phrase *
phrase_from_string (const gchar *str)
{
    Tokenizer *tok;
    gchar *term;
    Phrase *phrase;

    g_return_val_if_fail(str != NULL, NULL);

    if (index(str, '/') != NULL) {
        gchar **strs;
        guint idx;
        strs = g_strsplit(str, "/", 0);
        phrase = phrase_new();
        for (idx = 0; strs[idx] != NULL; idx++) {
            phrase_append(phrase, strs[idx]);
        }
        g_strfreev(strs);
    } else {
        tok = tokenizer_new(str);
        phrase = phrase_new();
        while(term = tokenizer_next(tok)){
            phrase_append_nocopy(phrase, term);
        }
        tokenizer_free(tok);
    }

    return phrase;
}

void
phrase_free (Phrase *phrase)
{
    guint i;
    for(i = 0;i < phrase->size;i++){
        g_free(phrase->terms[i]);
    }
    g_free(phrase->terms);
    g_free(phrase);
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
    phrase->terms[phrase->size - 1] = g_strdup(term);
}

static inline void
phrase_append_nocopy (Phrase *phrase, gchar *term)
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
