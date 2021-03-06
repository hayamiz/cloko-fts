#include "inv-index.h"
#include <string.h>
#include <stdlib.h>

Document *
document_parse (DocumentSet *docset, const gchar *text, guint offset, gint doc_id, const gchar **endptr)
{
    Document *doc = g_malloc(sizeof(Document));
    const gchar *ptr;
    const gchar *next_ptr;

    doc->id = doc_id;
    doc->pos = offset;
    doc->docset = docset;

    ptr = text + offset;
    if (*ptr != '#' || *(++ptr) != ' '){
        g_printerr("failed to find # mark\n");
        goto failure;
    }
    ptr++;
    if ((next_ptr = index(ptr, ' ')) == NULL){
        g_printerr("failed to find url top\n");
        goto failure;
    }
    doc->url_top = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, ' ')) == NULL){
        g_printerr("failed to find url\n");
        goto failure;
    }
    doc->url = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, ' ')) == NULL){
        g_printerr("failed to find title\n");
        goto failure;
    }
    doc->title = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, '\n')) == NULL){
        g_printerr("failed to parse retrieval time\n");
        goto failure;
    }
    doc->time = strtol(ptr, NULL, 10);
    ptr = next_ptr + 1;

    doc->body_pointer_offset = ptr - text;
    if ((next_ptr = strstr(ptr, "\nEOD")) == NULL){
        g_printerr("failed to find EOD mark\n");
        goto failure;
    }
    next_ptr += 5; // include '\nEOD\n'
    doc->size = next_ptr - (text + offset);
    doc->body_size = next_ptr - ptr - 5;

    *endptr = next_ptr;

    return doc;

failure:
    g_free(doc);
    return NULL;
}

void
document_free (Document *doc)
{
    g_return_if_fail(doc);

    g_free(doc->url);
    g_free(doc->url_top);
    g_free(doc->title);
    g_free(doc);
}

gint
document_id (Document *doc)
{
    return (doc ? doc->id : -1);
}

guint
document_position     (Document *doc)
{
    return (doc ? doc->pos : 0);
}

const gchar *
document_body_pointer (Document *doc)
{
    return (doc ? doc->docset->buffer + doc->body_pointer_offset : NULL);
}

guint
document_body_size    (Document *doc)
{
    return (doc != NULL ? doc->body_size : 0);
}

const gchar *
document_title        (Document *doc)
{
    return (doc ? doc->title : NULL);
}

const gchar *
document_url          (Document *doc)
{
    return (doc ? doc->url : NULL);
}

guint
document_time         (Document *doc)
{
    return (doc ? doc->time : 0);
}

gchar *
document_raw_record   (Document *doc)
{
    if (doc->docset){
        return (doc ? g_strndup(doc->docset->buffer + doc->pos, doc->size) : NULL);
    } else {
        return NULL;
    }
}
