#include "test.h"

void test_posting_pair_compare_func (void);
void test_posting_pair_new (void);

void test_posting_list_new (void);
void test_posting_list_copy (void);
void test_posting_list_add (void);
void test_posting_list_select_successor (void);
void test_posting_list_check (void);


void
test_posting_pair_compare_func (void)
{
    PostingPair p1, p2;

    p1.doc_id = p2.doc_id = 0;
    p1.pos    = p2.pos    = 0;
    cut_assert_equal_int(0, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = p2.doc_id = 99;
    p1.pos    = p2.pos    = 99;
    cut_assert_equal_int(0, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = 0;
    p2.doc_id = 1;
    p1.pos    = p2.pos    = 0;
    cut_assert_operator_int(0, >, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = 1;
    p2.doc_id = 0;
    p1.pos    = p2.pos    = 0;
    cut_assert_operator_int(0, <, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = p2.doc_id = 0;
    p1.pos    = 0;
    p2.pos    = 1;
    cut_assert_operator_int(0, >, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = p2.doc_id = 0;
    p1.pos    = 1;
    p2.pos    = 0;
    cut_assert_operator_int(0, <, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = 0;
    p2.doc_id = 1;
    p1.pos    = 2;
    p2.pos    = 3;
    cut_assert_operator_int(0, >, posting_pair_compare_func(&p1, &p2));

    p1.doc_id = 0;
    p2.doc_id = 1;
    p1.pos    = 3;
    p2.pos    = 2;
    cut_assert_operator_int(0, >, posting_pair_compare_func(&p1, &p2));
}

void
test_posting_pair_new (void)
{
    PostingPair *pair;
    pair = posting_pair_new(0, 0);
    cut_assert_equal_int(0, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);
    posting_pair_free(pair);

    pair = posting_pair_new(1, 1);
    cut_assert_equal_int(1, pair->doc_id);
    cut_assert_equal_int(1, pair->pos);
    posting_pair_free(pair);

    pair = posting_pair_new(3, 4);
    cut_assert_equal_int(3, pair->doc_id);
    cut_assert_equal_int(4, pair->pos);
    posting_pair_free(pair);

    pair = posting_pair_new(3, -4);
    cut_assert_equal_int(3, pair->doc_id);
    cut_assert_equal_int(-4, pair->pos);
    posting_pair_free(pair);
}

void
test_posting_list_new (void)
{
    PostingList *posting_list;
    posting_list = posting_list_new();
    cut_assert_not_null(posting_list);
    cut_assert_equal_uint(0, posting_list_size(posting_list));
}

void
test_posting_list_copy (void)
{
    PostingList *original_list;
    PostingList *list;
    original_list = posting_list_new();
    posting_list_add(original_list, 0, 0);
    posting_list_add(original_list, 1, 2);

    list = posting_list_copy(original_list);
    cut_assert(list != original_list);
    cut_assert_equal_uint(2, posting_list_size(list));
    cut_assert_not_null(posting_list_check(list, 0, 0));
    cut_assert_not_null(posting_list_check(list, 0, 0));

    posting_list_add(original_list, 4, 5);
    cut_assert_equal_uint(3, posting_list_size(original_list));
    cut_assert_equal_uint(2, posting_list_size(list));
}

void
test_posting_list_add (void)
{
    PostingList *posting_list;
    PostingPair *pair;

    posting_list = posting_list_new();

    posting_list_add(posting_list, 0, 1);
    cut_assert_equal_uint(1, posting_list_size(posting_list));
    posting_list_add(posting_list, 1, 12);
    cut_assert_equal_uint(2, posting_list_size(posting_list));
    posting_list_add(posting_list, 1, 31);
    cut_assert_equal_uint(3, posting_list_size(posting_list));
    posting_list_add(posting_list, 0, 21);
    cut_assert_equal_uint(4, posting_list_size(posting_list));
    posting_list_add(posting_list, 1, 1);
    cut_assert_equal_uint(5, posting_list_size(posting_list));
    posting_list_add(posting_list, 0, 11);
    cut_assert_equal_uint(6, posting_list_size(posting_list));

    cut_assert_not_null(posting_list_check(posting_list, 0, 1));
    cut_assert_not_null(posting_list_check(posting_list, 0, 11));
    cut_assert_not_null(posting_list_check(posting_list, 0, 21));
    cut_assert_not_null(posting_list_check(posting_list, 1, 1));
    cut_assert_not_null(posting_list_check(posting_list, 1, 12));
    cut_assert_not_null(posting_list_check(posting_list, 1, 31));

    posting_list_add(posting_list, 0, 1);
    cut_assert_equal_uint(6, posting_list_size(posting_list));
}

void
test_posting_list_select_successor (void)
{
    PostingList *list1;
    PostingList *list2;
    PostingList *list3;
    PostingList *list4;
    PostingList *list5;
    PostingList *list6;

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

    PostingList *base_list      = list1;
    PostingList *successor_list = list2; // the posting list of "is"
    guint         offset         = 1;
    base_list = posting_list_select_successor(base_list, successor_list, offset);
    cut_assert_not_null(base_list);
    cut_assert_equal_uint(2, posting_list_size(base_list));

    successor_list = list3; // the posting list of "an"
    offset         = 2;
    base_list = posting_list_select_successor(base_list, successor_list, offset);
    cut_assert_equal_uint(1, posting_list_size(base_list));

    successor_list = list4; // the posting list of "apple"
    offset         = 3;
    base_list = posting_list_select_successor(base_list, successor_list, offset);
    cut_assert_equal_uint(1, posting_list_size(base_list));

    successor_list = list6; // the posting list of "pen"
    offset         = 3;
    base_list = posting_list_select_successor(base_list, successor_list, offset);
    cut_assert_equal_uint(0, posting_list_size(base_list));

    posting_list_free(list1, TRUE);
    posting_list_free(list2, TRUE);
    posting_list_free(list3, TRUE);
    posting_list_free(list4, TRUE);
    posting_list_free(list5, TRUE);
    posting_list_free(list6, TRUE);
}

void
test_posting_list_check (void)
{
    PostingList *list = NULL;
    PostingPair *pair = NULL;

    cut_assert_null(posting_list_check(list, 0, 0));
    list = posting_list_new();
    cut_assert_null(posting_list_check(list, 0, 0));

    posting_list_add(list, 0, 0);
    posting_list_add(list, 2, 1);

    cut_assert_not_null(posting_list_check(list, 0, 0));
    cut_assert_not_null(posting_list_check(list, 2, 1));
    cut_assert_null(posting_list_check(list, 2, 3));
}

