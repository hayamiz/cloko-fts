#include "inv-index.h"
#include <stdio.h>

/* limit: the number of documents loaded. 0 means unlimited. */
DocumentSet *
document_set_load (const gchar *path, guint limit)
{
    DocumentSet *docset;
    GString *strbuffer;
    char buf[4096];
    guint endptr_idx;
    const gchar *endptr;
    const gchar *sentinel;
    FILE *fp;
    guint doc_id;
    Document  *doc;

    docset = g_malloc(sizeof(DocumentSet));
    strbuffer = g_string_new("");

    fp = fopen(path, "r");
    if (fp == NULL){
        return NULL;
    }

    doc_id = 0;
    docset->size = 0;
    docset->docs = NULL;
    endptr_idx = 0;
    endptr = strbuffer->str;
    while(fgets(buf, 4096 - 1, fp) != NULL){
        g_string_append(strbuffer, buf);
        if (strstr(strbuffer->str + endptr_idx, "EOD\n") == NULL){
            continue;
        }

        doc = document_parse(docset, strbuffer->str, endptr_idx, doc_id++, &endptr);
        if (doc == NULL) break;
        endptr_idx = endptr - strbuffer->str;
        docset->size++;
        docset->docs = g_realloc(docset->docs, sizeof(Document *) * docset->size);
        docset->docs[docset->size - 1] = doc;
        if (limit > 0 && doc_id == limit){
            break;
        }
    }
    fclose(fp);

    docset->buffer = strbuffer->str;
    docset->buffer_size = endptr - strbuffer->str;
    g_string_free(strbuffer, FALSE);

    return docset;
}

DocumentSet *
document_set_free (DocumentSet *docset)
{
    guint i;
    g_free(docset->buffer);
    for (i = 0; i < docset->size; i++) {
        document_free(docset->docs[i]);
    }
    g_free(docset->docs);
    g_free(docset);
}

guint document_set_size(DocumentSet *docset)
{
    return docset->size;
}

Document *
document_set_nth (DocumentSet *docset, guint idx)
{
    return (idx >= docset->size ? NULL : docset->docs[idx]);
}

const gchar *
document_set_buffer (DocumentSet *docset)
{
    return docset->buffer;
}

