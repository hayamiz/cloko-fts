#include <cutter.h>
#include <inv-index.h>

void test_posting_pair_compare_func (void);

void test_posting_list_new (void);
void test_posting_list_add (void);

void test_new_inv_index (void);
void test_inv_index_add (void);
void test_inv_index_get (void);


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
test_posting_list_add (void)
{
    PostingList *posting_list;
    PostingPair *pair;
    GSList *list;

    posting_list = posting_list_new();

    posting_list_add(posting_list, 0, 1);
    posting_list_add(posting_list, 1, 12);
    posting_list_add(posting_list, 1, 31);
    posting_list_add(posting_list, 0, 21);
    posting_list_add(posting_list, 1, 1);
    posting_list_add(posting_list, 0, 11);

    list = posting_list_get_slist(posting_list);

    cut_assert_equal_uint(6, g_slist_length(list));

    pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(1, pair->pos);

    pair = (PostingPair *) g_slist_nth_data(list, 1);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(11, pair->pos);

    pair = (PostingPair *) g_slist_nth_data(list, 2);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(21, pair->pos);

    pair = (PostingPair *) g_slist_nth_data(list, 3);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(1, pair->pos);

    pair = (PostingPair *) g_slist_nth_data(list, 4);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(12, pair->pos);

    pair = (PostingPair *) g_slist_nth_data(list, 5);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(31, pair->pos);
}



void
test_new_inv_index (void)
{
    InvIndex *inv_index;
    inv_index = inv_index_new();
    cut_assert_not_null(inv_index);
    cut_assert_equal_uint(0, inv_index_numterms(inv_index));
}

void
test_inv_index_add (void)
{
    InvIndex    *inv_index;
    const gchar *term    = "foo";
    gint        doc_id1 = 0;
    gint        pos1 = 10;
    gint        doc_id2 = 1;
    gint        pos2 = 20;

    inv_index = inv_index_new();
    cut_assert_equal_int(0, inv_index_numterms(inv_index));
    inv_index_add_term(inv_index, term, doc_id1, pos1);
    cut_assert_equal_int(1, inv_index_numterms(inv_index));
    inv_index_add_term(inv_index, term, doc_id2, pos2);
    cut_assert_equal_int(1, inv_index_numterms(inv_index));
}

void
test_inv_index_get (void)
{
    InvIndex *inv_index;
    PostingList *posting_list;
    PostingPair *posting_pair;
    GSList *list;

    const gchar *term1 = "foo";
    const gchar *term2 = "bar";
    const gchar *term3 = "baz";
    const gchar *term4 = "qux";

    inv_index = inv_index_new();

    inv_index_add_term(inv_index, term4, 2, 3);
    inv_index_add_term(inv_index, term4, 3, 3);
    inv_index_add_term(inv_index, term3, 0, 2);
    inv_index_add_term(inv_index, term3, 1, 2);
    inv_index_add_term(inv_index, term3, 2, 2);
    inv_index_add_term(inv_index, term2, 1, 1);
    inv_index_add_term(inv_index, term1, 0, 0);
    inv_index_add_term(inv_index, term1, 2, 0);

    cut_assert_null(inv_index_get(inv_index, "hoge"));

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term1));
    list = posting_list_get_slist(posting_list);
    cut_assert_equal_uint(2, g_slist_length(list));
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(0, posting_pair->doc_id);
    cut_assert_equal_int(0, posting_pair->pos);
    list = g_slist_next(list);
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(0, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term2));
    list = posting_list_get_slist(posting_list);
    cut_assert_equal_uint(1, g_slist_length(list));
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(1, posting_pair->doc_id);
    cut_assert_equal_int(1, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term3));
    list = posting_list_get_slist(posting_list);
    cut_assert_equal_uint(3, g_slist_length(list));
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(0, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);
    list = g_slist_next(list);
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(1, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);
    list = g_slist_next(list);
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term4));
    list = posting_list_get_slist(posting_list);
    cut_assert_equal_uint(2, g_slist_length(list));
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(3, posting_pair->pos);
    list = g_slist_next(list);
    posting_pair = (PostingPair *) g_slist_nth_data(list, 0);
    cut_assert_equal_int(3, posting_pair->doc_id);
    cut_assert_equal_int(3, posting_pair->pos);

}
