#include "test.h"


void test_fixed_posting_list_new (void);
void test_fixed_posting_list_copy (void);
void test_fixed_posting_list_add (void);
void test_fixed_posting_list_select_successor (void);
void test_fixed_posting_list_check (void);

void
test_fixed_posting_list_new (void)
{
    PostingList *posting_list;
    FixedPostingList *fplist;
    posting_list = posting_list_new();
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(0, fixed_posting_list_size(fplist));
    posting_list_free(posting_list, TRUE);
    fixed_posting_list_free(fplist);
}

void
test_fixed_posting_list_copy (void)
{
    PostingList *original_list;
    FixedPostingList *ofplist;
    FixedPostingList *fplist;
    original_list = posting_list_new();
    posting_list_add(original_list, 0, 0);
    posting_list_add(original_list, 1, 2);
    ofplist = fixed_posting_list_new(original_list);

    fplist = fixed_posting_list_copy(ofplist);
    cut_assert(fplist != ofplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));

    posting_list_add(original_list, 4, 5);
    fixed_posting_list_free(ofplist);
    ofplist = fixed_posting_list_new(original_list);
    cut_assert_equal_uint(3, fixed_posting_list_size(ofplist));
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
}

void
test_fixed_posting_list_add (void)
{
    PostingList *posting_list;
    FixedPostingList *fplist;
    PostingPair *pair;

    posting_list = posting_list_new();

    posting_list_add(posting_list, 0, 1);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
    posting_list_add(posting_list, 1, 12);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
    posting_list_add(posting_list, 1, 31);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(3, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
    posting_list_add(posting_list, 0, 21);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(4, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
    posting_list_add(posting_list, 1, 1);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(5, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
    posting_list_add(posting_list, 0, 11);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(6, fixed_posting_list_size(fplist));

    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 1));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 11));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 21));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 1));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 12));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 31));
    fixed_posting_list_free(fplist);

    posting_list_add(posting_list, 0, 1);
    fplist = fixed_posting_list_new(posting_list);
    cut_assert_equal_uint(6, fixed_posting_list_size(fplist));
    fixed_posting_list_free(fplist);
}

void
test_fixed_posting_list_select_successor (void)
{
    PostingList *list1;
    PostingList *list2;
    PostingList *list3;
    PostingList *list4;
    PostingList *list5;
    PostingList *list6;

    FixedPostingList *fplist1;
    FixedPostingList *fplist2;
    FixedPostingList *fplist3;
    FixedPostingList *fplist4;
    FixedPostingList *fplist5;
    FixedPostingList *fplist6;

    list1 = posting_list_new();
    list2 = posting_list_new();
    list3 = posting_list_new();
    list4 = posting_list_new();
    list5 = posting_list_new();
    list6 = posting_list_new();

    posting_list_add(list1, 0, 0); // "this"
    posting_list_add(list2, 0, 1); // "is"
    posting_list_add(list3, 0, 2); // "an"
    posting_list_add(list4, 0, 3); // "apple"

    posting_list_add(list1, 1, 0); // "this"
    posting_list_add(list2, 1, 1); // "is"
    posting_list_add(list5, 1, 2); // "a"
    posting_list_add(list6, 1, 3); // "pen"

    fplist1 = fixed_posting_list_new(list1);
    fplist2 = fixed_posting_list_new(list2);
    fplist3 = fixed_posting_list_new(list3);
    fplist4 = fixed_posting_list_new(list4);
    fplist5 = fixed_posting_list_new(list5);
    fplist6 = fixed_posting_list_new(list6);

    FixedPostingList *base_list      = fplist1;
    FixedPostingList *succ_list = fplist2; // the posting list of "is"
    guint         offset         = 1;
    base_list = fixed_posting_list_select_successor(base_list, succ_list, offset);
    cut_assert_not_null(base_list);
    cut_assert_equal_uint(2, fixed_posting_list_size(base_list));

    succ_list = fplist3; // the posting list of "an"
    offset         = 2;
    base_list = fixed_posting_list_select_successor(base_list, succ_list, offset);
    cut_assert_equal_uint(1, fixed_posting_list_size(base_list));

    succ_list = fplist4; // the posting list of "apple"
    offset         = 3;
    base_list = fixed_posting_list_select_successor(base_list, succ_list, offset);
    cut_assert_equal_uint(1, fixed_posting_list_size(base_list));

    succ_list = fplist6; // the posting list of "pen"
    offset         = 3;
    base_list = fixed_posting_list_select_successor(base_list, succ_list, offset);
    cut_assert_equal_uint(0, fixed_posting_list_size(base_list));
}
