
#include "test.h"


static Phrase *phrase;

void test_phrase_new (void);
void test_phrase_append (void);
void test_phrase_nth (void);

void test_new_inv_index (void);
void test_inv_index_add (void);
void test_inv_index_get (void);
void test_inv_index_phrase_get (void);


void
cut_setup (void)
{
    phrase = NULL;
}

void
cut_teardown (void)
{
    if (phrase)
        phrase_free(phrase);
}

void
test_phrase_new (void)
{
    phrase = phrase_new();
    cut_assert_not_null(phrase);
    cut_assert_equal_uint(0, phrase_size(phrase));
}

void
test_phrase_append (void)
{
    phrase = phrase_new();
    phrase_append(phrase, "foo");
    cut_assert_equal_uint(1, phrase_size(phrase));

    phrase_append(phrase, "bar");
    cut_assert_equal_uint(2, phrase_size(phrase));
}

void
test_phrase_nth (void)
{
    phrase = phrase_new();
    cut_assert_null(phrase_nth(phrase, 0));

    phrase_append(phrase, "foo");
    phrase_append(phrase, "bar");

    cut_assert_equal_string("foo", phrase_nth(phrase, 0));
    cut_assert_equal_string("bar", phrase_nth(phrase, 1));
    cut_assert_null(phrase_nth(phrase, 2));
}

void
test_new_inv_index (void)
{
    InvIndex *inv_index;
    inv_index = inv_index_new();
    cut_assert_not_null(inv_index);
    cut_assert_equal_uint(0, inv_index_numterms(inv_index));
    inv_index_free(inv_index);
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
    cut_assert_not_null(posting_list_check(posting_list, 0, 0));
    cut_assert_not_null(posting_list_check(posting_list, 2, 0));

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term2));
    cut_assert_equal_uint(1, posting_list_size(posting_list));
    cut_assert_not_null(posting_list_check(posting_list, 1, 1));

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term3));
    cut_assert_equal_uint(3, posting_list_size(posting_list));
    cut_assert_not_null(posting_list_check(posting_list, 0, 2));
    cut_assert_not_null(posting_list_check(posting_list, 1, 2));
    cut_assert_not_null(posting_list_check(posting_list, 2, 2));

    cut_assert_not_null(posting_list = inv_index_get(inv_index, term4));
    cut_assert_equal_uint(2, posting_list_size(posting_list));
    cut_assert_not_null(posting_list_check(posting_list, 2, 3));
    cut_assert_not_null(posting_list_check(posting_list, 3, 3));

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
    cut_assert_not_null(posting_list_check(list, 0, 0));
    cut_assert_not_null(posting_list_check(list, 1, 0));

    phrase_append(phrase, "is");

    list = inv_index_phrase_get(inv_index, phrase);
    cut_assert_not_null(list);
    cut_assert_equal_uint(2, posting_list_size(list));

    cut_assert_not_null(posting_list_check(list, 0, 0));
    cut_assert_not_null(posting_list_check(list, 1, 0));

    // check posting is not destructed
    list = inv_index_get(inv_index, "this");
    cut_assert_equal_uint(2, posting_list_size(list));
    cut_assert_not_null(posting_list_check(list, 0, 0));
    cut_assert_not_null(posting_list_check(list, 1, 0));

    phrase_append(phrase, "a");

    list = inv_index_phrase_get(inv_index, phrase);
    cut_assert_equal_uint(1, posting_list_size(list));
    cut_assert_not_null(posting_list_check(list, 1, 0));
}
