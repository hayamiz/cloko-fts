
#include "inv-index.h"


Phrase *
phrase_new (void)
{
    Phrase *phrase = g_malloc(sizeof(Phrase));
    phrase->size = 0;
    phrase->terms = NULL;

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

const gchar *
phrase_nth (Phrase *phrase, guint idx)
{
    if (idx >= phrase_size(phrase)){
        return NULL;
    }

    return phrase->terms[idx];
}
