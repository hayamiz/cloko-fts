#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <glib.h>
#include <stdlib.h>

#define SKIPLIST_MAX_LEVEL 5

typedef struct _Skipnode Skipnode;
struct _Skipnode {
    guint64 data;
    gint level;
    Skipnode **skips;
};

typedef struct _Skiplist {
    Skipnode *head;
    Skipnode *tail;
    Skipnode *skips[SKIPLIST_MAX_LEVEL];
    guint length;
} Skiplist;

Skiplist *skiplist_new       (void);
void      skiplist_free      (Skiplist *list);
guint     skiplist_length    (Skiplist *list);
gboolean  skiplist_insert    (Skiplist *list, guint64 data);
gboolean  skiplist_exists    (Skiplist *list, guint64 data);

#endif
