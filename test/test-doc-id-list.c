#include "test.h"

void test_doc_id_list_new (void);
void test_doc_id_list_intersect (void);

void
test_doc_id_list_new (void)
{
    PostingList *plist = posting_list_new();
    FixedPostingList *fplist = fixed_posting_list_new(plist);
    DocIdList *dlist = doc_id_list_new(fplist);

    cut_assert_not_null(dlist);
    cut_assert_equal_uint(0, doc_id_list_size(dlist));

    posting_list_free(plist, TRUE);
    fixed_posting_list_free(fplist);
    doc_id_list_free(dlist);

    plist = posting_list_new();
    posting_list_add(plist, 0, 0);
    posting_list_add(plist, 0, 1);
    posting_list_add(plist, 1, 3);
    posting_list_add(plist, 3, 4);
    fplist = fixed_posting_list_new(plist);
    dlist = doc_id_list_new(fplist);

    cut_assert_not_null(dlist);
    cut_assert_equal_uint(3, doc_id_list_size(dlist));

    posting_list_free(plist, TRUE);
    fixed_posting_list_free(fplist);
    doc_id_list_free(dlist);
}

void
test_doc_id_list_intersect (void)
{
    PostingList *plist1 = posting_list_new();
    posting_list_add(plist1, 0, 0);
    posting_list_add(plist1, 0, 1);
    posting_list_add(plist1, 1, 3);
    posting_list_add(plist1, 3, 4);
    posting_list_add(plist1, 200, 4);
    posting_list_add(plist1, 100000, 4);
    PostingList *plist2 = posting_list_new();
    posting_list_add(plist2, 0, 0);
    posting_list_add(plist2, 1, 1);
    posting_list_add(plist2, 2, 3);
    posting_list_add(plist2, 3, 4);
    posting_list_add(plist2, 200, 4);
    posting_list_add(plist2, 100001, 4);

    FixedPostingList *fplist1 = fixed_posting_list_new(plist1);
    FixedPostingList *fplist2 = fixed_posting_list_new(plist2);
    DocIdList *dlist1 = doc_id_list_new(fplist1);
    DocIdList *dlist2 = doc_id_list_new(fplist2);
    cut_assert_equal_uint(5, doc_id_list_size(dlist1));
    cut_assert_equal_uint(6, doc_id_list_size(dlist2));

    DocIdList *dlist = doc_id_list_intersect(dlist1, dlist2);
    doc_id_list_free(dlist1);
    doc_id_list_free(dlist2);

    cut_assert_equal_uint(4, doc_id_list_size(dlist));
    cut_assert(doc_id_list_check(dlist, 0));
    cut_assert(doc_id_list_check(dlist, 1));
    cut_assert(doc_id_list_check(dlist, 3));
    cut_assert(doc_id_list_check(dlist, 200));
}
