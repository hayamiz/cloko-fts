
#include <inv-index.h>

DocIdList *
doc_id_list_new(FixedPostingList *fplist)
{
    DocIdList *dlist = g_malloc(sizeof(DocIdList));
    
    return NULL;
}

guint
doc_id_list_size(DocIdList *dlist)
{
    // TODO
    return 0;
}

DocIdList *doc_id_list_free(DocIdList *list)
{
    // TODO
    return NULL;
}

DocIdList *doc_id_list_intersect(DocIdList* list1, DocIdList* list2)
{
    // TODO
    return NULL;
}

gboolean   doc_id_list_check(DocIdList *list, guint doc_id)
{
    // TODO
    return FALSE;
}

