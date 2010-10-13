#include "inv-index.h"
#include <stdio.h>

DocumentSet *
document_set_load (const gchar *path)
{
    DocumentSet *docset = g_malloc(sizeof(DocumentSet));
    GString *strbuffer = g_string_new("");
    char buf[4096];

    FILE *fp = fopen(path, "r");
    if (fp == NULL){
        return NULL;
    }
    while(fgets(buf, 4096 - 1, fp) != NULL){
        g_string_append(strbuffer, buf);
    }
    fclose(fp);

    docset->buffer = strbuffer->str;
    docset->buffer_size = strbuffer->len;
    g_string_free(strbuffer, FALSE);

    Document  *doc;
    docset->size = 0;
    docset->docs = NULL;
    const gchar *endptr = docset->buffer;
    const gchar *sentinel = docset->buffer + docset->buffer_size;
    while(endptr < sentinel){
        doc = document_parse(docset->buffer, endptr - docset->buffer, docset->size, &endptr);
        if (doc == NULL){
            break;
        }
        docset->size++;
        docset->docs = g_realloc(docset->docs, sizeof(Document *) * docset->size);
        docset->docs[docset->size - 1] = doc;
    }


    return docset;
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

