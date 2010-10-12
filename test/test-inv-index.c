#include <cutter.h>
#include <inv-index.h>

void test_phrase_new (void);
void test_phrase_append (void);
void test_phrase_nth (void);

void test_posting_pair_compare_func (void);

void test_posting_list_new (void);
void test_posting_list_copy (void);
void test_posting_list_add (void);
void test_posting_list_select_successor (void);
void test_posting_list_nth (void);

void test_new_inv_index (void);
void test_inv_index_add (void);
void test_inv_index_get (void);



void
test_phrase_new (void)
{
    Phrase *phrase;
    phrase = phrase_new();
    cut_assert_not_null(phrase);
    cut_assert_equal_uint(0, phrase_size(phrase));
}

void
test_phrase_append (void)
{
    Phrase *phrase;
    phrase = phrase_new();
    phrase_append(phrase, "foo");
    cut_assert_equal_uint(1, phrase_size(phrase));

    phrase_append(phrase, "bar");
    cut_assert_equal_uint(2, phrase_size(phrase));
}

void
test_phrase_nth (void)
{
    Phrase *phrase;
    phrase = phrase_new();
    cut_assert_null(phrase_nth(phrase, 0));

    phrase_append(phrase, "foo");
    phrase_append(phrase, "bar");

    cut_assert_equal_string("foo", phrase_nth(phrase, 0));
    cut_assert_equal_string("bar", phrase_nth(phrase, 1));
    cut_assert_null(phrase_nth(phrase, 2));
}

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
test_posting_list_copy (void)
{
    PostingList *original_list;
    PostingList *list;
    original_list = posting_list_new();
    posting_list_add(original_list, 0, 0);
    posting_list_add(original_list, 1, 2);

    list = posting_list_copy(original_list);
    cut_assert(list != original_list);
    cut_assert_equal_uint(2, posting_list_size(original_list));
    cut_assert_equal_uint(2, posting_list_size(list));
    cut_assert_equal_int(0, posting_pair_compare_func(posting_list_nth(original_list, 0),
                                                      posting_list_nth(list, 0)));
    cut_assert_equal_int(0, posting_pair_compare_func(posting_list_nth(original_list, 1),
                                                      posting_list_nth(list, 1)));

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

    pair = posting_list_nth(posting_list, 0);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(1, pair->pos);

    pair = posting_list_nth(posting_list, 1);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(11, pair->pos);

    pair = posting_list_nth(posting_list, 2);
    cut_assert_equal_uint(0, pair->doc_id);
    cut_assert_equal_uint(21, pair->pos);

    pair = posting_list_nth(posting_list, 3);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(1, pair->pos);

    pair = posting_list_nth(posting_list, 4);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(12, pair->pos);

    pair = posting_list_nth(posting_list, 5);
    cut_assert_equal_uint(1, pair->doc_id);
    cut_assert_equal_uint(31, pair->pos);

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
}

void
test_posting_list_nth (void)
{
    PostingList *list = NULL;
    PostingPair *pair = NULL;

    cut_assert_null(posting_list_nth(list, 0));
    list = posting_list_new();
    cut_assert_null(posting_list_nth(list, 0));

    posting_list_add(list, 0, 0);
    posting_list_add(list, 2, 1);

    pair = posting_list_nth(list, 0);
    cut_assert_not_null(pair);
    cut_assert_equal_int(0, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);

    pair = posting_list_nth(list, 1);
    cut_assert_not_null(pair);
    cut_assert_equal_int(2, pair->doc_id);
    cut_assert_equal_int(1, pair->pos);

    cut_assert_null(posting_list_nth(list, 2));
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
    cut_assert_equal_uint(2, posting_list_size(posting_list));
    posting_pair = posting_list_nth(posting_list, 0);
    cut_assert_equal_int(0, posting_pair->doc_id);
    cut_assert_equal_int(0, posting_pair->pos);
    posting_pair = posting_list_nth(posting_list, 1);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(0, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term2));
    cut_assert_equal_uint(1, posting_list_size(posting_list));
    posting_pair = posting_list_nth(posting_list, 0);
    cut_assert_equal_int(1, posting_pair->doc_id);
    cut_assert_equal_int(1, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term3));
    cut_assert_equal_uint(3, posting_list_size(posting_list));
    posting_pair = posting_list_nth(posting_list, 0);
    cut_assert_equal_int(0, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);
    posting_pair = posting_list_nth(posting_list, 1);
    cut_assert_equal_int(1, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);
    posting_pair = posting_list_nth(posting_list, 2);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(2, posting_pair->pos);

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term4));
    cut_assert_equal_uint(2, posting_list_size(posting_list));
    posting_pair = posting_list_nth(posting_list, 0);
    cut_assert_equal_int(2, posting_pair->doc_id);
    cut_assert_equal_int(3, posting_pair->pos);
    posting_pair = posting_list_nth(posting_list, 1);
    cut_assert_equal_int(3, posting_pair->doc_id);
    cut_assert_equal_int(3, posting_pair->pos);

}

void
test_inv_index_phrase_get (void)
{
    InvIndex    *inv_index;
    Phrase      *phrase;
    PostingList *list;
    PostingPair *pair;

    inv_index = inv_index_new();
    inv_index_add_term(inv_index, "this" , 0, 0);
    inv_index_add_term(inv_index, "is"   , 0, 1);
    inv_index_add_term(inv_index, "an"   , 0, 2);
    inv_index_add_term(inv_index, "apple", 0, 3);

    inv_index_add_term(inv_index, "this", 1, 0);
    inv_index_add_term(inv_index, "is"  , 1, 1);
    inv_index_add_term(inv_index, "a"   , 1, 2);
    inv_index_add_term(inv_index, "pen" , 1, 3);

    phrase = phrase_new();
    phrase_append(phrase, "this");
    list = inv_index_phrase_get(inv_index, phrase);
    cut_assert_not_null(list);
    cut_assert_equal_uint(2, posting_list_size(list));
    pair = posting_list_nth(list, 0);
    cut_assert_equal_int(0, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);


    phrase_append(phrase, "is");

    list = inv_index_phrase_get(inv_index, phrase);
    cut_assert_not_null(list);
    cut_assert_equal_uint(2, posting_list_size(list));

    pair = posting_list_nth(list, 0);
    cut_assert_equal_int(0, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);

    pair = posting_list_nth(list, 1);
    cut_assert_equal_int(1, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);

    // check posting is not destructed
    list = inv_index_get(inv_index, "this");
    cut_assert_equal_uint(2, posting_list_size(list));


    phrase_append(phrase, "a");

    list = inv_index_phrase_get(inv_index, phrase);
    cut_assert_equal_uint(1, posting_list_size(list));
    pair = posting_list_nth(list, 0);
    cut_assert_equal_int(1, pair->doc_id);
    cut_assert_equal_int(0, pair->pos);
}
