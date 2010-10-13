#include "inv-index.h"
#include <string.h>
#include <stdlib.h>

Document *
document_parse (const gchar *text, guint offset, gint doc_id, const gchar **endptr)
{
    Document *doc = g_malloc(sizeof(Document));
    const gchar *ptr;
    const gchar *next_ptr;

    doc->id = doc_id;
    doc->pos = offset;
    doc->docset_buf = text;

    ptr = text + offset;
    if (*ptr != '#' || *(++ptr) != ' '){
        goto failure;
    }
    ptr++;
    if ((next_ptr = index(ptr, ' ')) == NULL){
        goto failure;
    }
    doc->url_top = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, ' ')) == NULL){
        goto failure;
    }
    doc->url = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, ' ')) == NULL){
        goto failure;
    }
    doc->title = g_strndup(ptr, next_ptr - ptr);
    ptr = next_ptr + 1;

    if ((next_ptr = index(ptr, '\n')) == NULL){
        goto failure;
    }
    doc->time = strtol(ptr, NULL, 10);
    ptr = next_ptr + 1;

    doc->body_pointer = ptr;
    if ((next_ptr = strstr(ptr, "\nEOD\n")) == NULL){
        goto failure;
    }
    doc->size = next_ptr - (text + offset);
    doc->body_size = next_ptr - ptr;
    g_printerr("body_size = %d\n", next_ptr - ptr);

    *endptr = next_ptr + 5;

    return doc;

failure:
    g_free(doc);
    return NULL;
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
    return (doc ? doc->body_pointer : NULL);
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
    return (doc ? g_strndup(doc->docset_buf + doc->pos, doc->size + 5) : NULL);
}