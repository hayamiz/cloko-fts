
#include "test.h"

static const gchar *term1 = "foo";
static const gchar *term2 = "bar";
static const gchar *term3 = "baz";
static const gchar *term4 = "qux";
static const gchar *term5 = "quxx";
static InvIndex *sample_inv_index;
static FixedIndex *sample_fixed_index;

void cut_setup (void);

void test_fixed_index_new (void);
void test_fixed_index_get (void);
void test_fixed_index_phrase_get (void);
void test_fixed_index_load_dump (void);

void
cut_setup (void)
{
    sample_inv_index = inv_index_new();

    inv_index_add_term(sample_inv_index, term4, 2, 3);
    inv_index_add_term(sample_inv_index, term4, 3, 3);
    inv_index_add_term(sample_inv_index, term3, 0, 2);
    inv_index_add_term(sample_inv_index, term3, 1, 2);
    inv_index_add_term(sample_inv_index, term3, 2, 2);
    inv_index_add_term(sample_inv_index, term2, 1, 1);
    inv_index_add_term(sample_inv_index, term1, 0, 0);
    inv_index_add_term(sample_inv_index, term1, 2, 0);

    sample_fixed_index = fixed_index_new(sample_inv_index);
}

void
test_fixed_index_new (void)
{
    InvIndex *inv_index = inv_index_new();
    FixedIndex *fixed_index = fixed_index_new(inv_index);
    cut_assert_not_null(fixed_index);
    cut_assert_equal_uint(0, fixed_index_numterms(fixed_index));
    fixed_index_free(fixed_index);
    inv_index_free(inv_index);

    cut_assert_equal_uint(4, fixed_index_numterms(sample_fixed_index));
}

void
test_fixed_index_get (void)
{
    InvIndex *inv_index = inv_index_new();
    FixedIndex *fixed_index = fixed_index_new(inv_index);
    FixedPostingList *fplist;

    cut_assert_null(fixed_index_get(fixed_index, term1));

    fixed_index = sample_fixed_index;
    fplist = fixed_index_get(fixed_index, term1);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 0));

    fplist = fixed_index_get(fixed_index, term2);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 1));

    fplist = fixed_index_get(fixed_index, term3);
    cut_assert_equal_uint(3, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 2));

    fplist = fixed_index_get(fixed_index, term4);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 3));
    cut_assert_not_null(fixed_posting_list_check(fplist, 3, 3));

    fplist = fixed_index_get(fixed_index, term5);
    cut_assert_null(fplist);
}

void
test_fixed_index_phrase_get (void)
{
    InvIndex         *inv_index;
    FixedIndex       *fixed_index;
    Phrase           *phrase;
    FixedPostingList *fplist;

    inv_index = inv_index_new();
    inv_index_add_term(inv_index, "this" , 0, 0);
    inv_index_add_term(inv_index, "is"   , 0, 1);
    inv_index_add_term(inv_index, "an"   , 0, 2);
    inv_index_add_term(inv_index, "apple", 0, 3);

    inv_index_add_term(inv_index, "this", 1, 0);
    inv_index_add_term(inv_index, "is"  , 1, 1);
    inv_index_add_term(inv_index, "a"   , 1, 2);
    inv_index_add_term(inv_index, "pen" , 1, 3);

    fixed_index = fixed_index_new(inv_index);
    inv_index_free(inv_index);

    phrase = phrase_new();
    phrase_append(phrase, "this");
    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));

    phrase_append(phrase, "is");

    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));

    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));


    // check posting is not destructed
    fplist = fixed_index_get(fixed_index, "this");
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));

    phrase_append(phrase, "a");

    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));
}

void
test_fixed_index_load_dump (void)
{
    InvIndex *inv_index = inv_index_new();
    FixedIndex *fixed_index = fixed_index_new(inv_index);
    inv_index_free(inv_index);
    fixed_index_dump(fixed_index, "/tmp/fixed_index.dump");
    fixed_index_free(fixed_index);

    fixed_index = fixed_index_load("/tmp/fixed_index.dump");
    cut_assert_not_null(fixed_index);
    cut_assert_equal_uint(0, fixed_index_numterms(fixed_index));
    fixed_index_free(fixed_index);

    fixed_index_dump(sample_fixed_index, "/tmp/fixed_index.dump");
    fixed_index = fixed_index_load("/tmp/fixed_index.dump");

    FixedPostingList *fplist;

    fplist = fixed_index_get(fixed_index, term1);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 0));

    fplist = fixed_index_get(fixed_index, term2);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 1));

    fplist = fixed_index_get(fixed_index, term3);
    cut_assert_equal_uint(3, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 2));

    fplist = fixed_index_get(fixed_index, term4);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 3));
    cut_assert_not_null(fixed_posting_list_check(fplist, 3, 3));

    fplist = fixed_index_get(fixed_index, term5);
    cut_assert_null(fplist);
}
