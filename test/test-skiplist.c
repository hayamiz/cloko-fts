
#include "test.h"


static Skiplist *list;
static Skipnode *node;
static FixedPostingList *fplist;

void
cut_setup (void)
{
    list = NULL;
    fplist = NULL;
}

void
cut_teardown (void)
{
    if (list)
        skiplist_free(list);
    if (fplist)
        fixed_posting_list_free(fplist);
}

void
test_skiplist_new (void)
{
    list = skiplist_new();
    cut_assert_not_null(list);
}

void
test_skiplist_insert (void)
{
    list = skiplist_new();
    cut_assert_not_null(list);
    cut_assert_true(skiplist_insert(list, 9));
    cut_assert_true(skiplist_insert(list, 8));
    cut_assert_true(skiplist_insert(list, 100));
    cut_assert_true(skiplist_insert(list, 101));
    cut_assert_true(skiplist_insert(list, 2));
    cut_assert_true(skiplist_insert(list, 5));
    cut_assert_true(skiplist_insert(list, 58));
    cut_assert_true(skiplist_insert(list, 29));
    cut_assert_equal_uint(8, skiplist_length(list));
    cut_assert_false(skiplist_insert(list, 101));

    Skipnode *node;
    guint64 prev_data;
    prev_data = list->head->skips[0]->data;
    for (node = list->head->skips[0]->skips[0];
         node != NULL; node = node->skips[0]){
        cut_assert_operator_uint(prev_data, <, node->data);
    }
}

void
test_skiplist_length (void)
{
    list = skiplist_new();
    cut_assert_equal_uint(0, skiplist_length(list));

    skiplist_insert(list, 9);
    cut_assert_equal_uint(1, skiplist_length(list));

    skiplist_insert(list, 8);
    skiplist_insert(list, 100);
    skiplist_insert(list, 101);
    cut_assert_equal_uint(4, skiplist_length(list));

    skiplist_insert(list, 101);
    cut_assert_equal_uint(4, skiplist_length(list));

    skiplist_insert(list, 2);
    skiplist_insert(list, 5);
    skiplist_insert(list, 58);
    skiplist_insert(list, 29);

    cut_assert_equal_uint(8, skiplist_length(list));
}

void
test_skiplist_from_fixed_posting_list (void)
{
    PostingList *plist;
    PostingPair p;
    guint64 *pp;
    Skiplist *skip;
    plist = posting_list_new();
    posting_list_add(plist, 1, 2);
    posting_list_add(plist, 3, 4);
    posting_list_add(plist, 5, 8);
    posting_list_add(plist, 10, 2);
    fplist = fixed_posting_list_new(plist);
    posting_list_free(plist, TRUE);
    pp = (guint64 *)&p;

    list = fixed_posting_list_to_skiplist (fplist);
    cut_assert_not_null(list);

    p.doc_id = 1; p.pos = 2;
    cut_assert_true(skiplist_exists(list, *pp));

    p.doc_id = 3; p.pos = 4;
    cut_assert_true(skiplist_exists(list, *pp));

    p.doc_id = 5; p.pos = 8;
    cut_assert_true(skiplist_exists(list, *pp));

    p.doc_id = 10; p.pos = 2;
    cut_assert_true(skiplist_exists(list, *pp));
}

void
test_skiplist_intersect (void)
{
    Skiplist *list1, *list2;
    Skipnode *node;
    FixedPostingList *fplist;

    list1 = skiplist_new();
    skiplist_insert(list1, 9);
    skiplist_insert(list1, 8);
    skiplist_insert(list1, 100);
    skiplist_insert(list1, 101);
    skiplist_insert(list1, 2);
    skiplist_insert(list1, 5);
    skiplist_insert(list1, 58);
    skiplist_insert(list1, 29);
    skiplist_insert(list1, 101);
    cut_assert_not_null(list1);

    list2 = skiplist_new();
    skiplist_insert(list1, 7);
    skiplist_insert(list1, 8);
    skiplist_insert(list1, 101);
    skiplist_insert(list1, 2);
    skiplist_insert(list1, 120);
    skiplist_insert(list1, 5);
    skiplist_insert(list1, 58);
    skiplist_insert(list1, 2);
    skiplist_insert(list1, 1);
    cut_assert_not_null(list2);

    fplist = fixed_posting_list_from_skiplist_intersect(list1, list2);
    cut_assert_not_null(fplist);
    skiplist_free(list1);
    skiplist_free(list2);

    cut_assert_equal_uint(5, fixed_posting_list_size(fplist));
    cut_assert_true(fixed_posting_list_check(fplist, 0, 2));
    cut_assert_true(fixed_posting_list_check(fplist, 0, 5));
    cut_assert_true(fixed_posting_list_check(fplist, 0, 9));
    cut_assert_true(fixed_posting_list_check(fplist, 0, 58));
    cut_assert_true(fixed_posting_list_check(fplist, 0, 101));
}
