
#include "inv-index.h"

FixedPostingList *
fixed_posting_list_new (PostingList *list)
{
    // TODO
}

void
fixed_posting_list_free (FixedPostingList *fplist)
{
    // TODO
}

FixedPostingList *
fixed_posting_list_copy  (FixedPostingList *fixed_posting_list)
{
    // TODO
}

guint
fixed_posting_list_size  (FixedPostingList *fixed_posting_list)
{
    // TODO
}

PostingPair *
fixed_posting_list_check (FixedPostingList *fixed_posting_list, guint32 doc_id, gint32 pos)
{
    // TODO
}

FixedPostingList *
fixed_posting_list_select_successor (FixedPostingList *base_list,
                                     FixedPostingList *successor_list,
                                     guint offset)
{
    // TODO
}


