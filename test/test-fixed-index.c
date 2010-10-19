
#include "test.h"

static const gchar *term1 = "foo";
static const gchar *term2 = "bar";
static const gchar *term3 = "baz";
static const gchar *term4 = "qux";
static const gchar *term5 = "quxx";
static InvIndex *sample_inv_index;
static InvIndex *inv_index;
static FixedIndex *fixed_index;
static Phrase *phrase;
static FixedPostingList *fplist;


void cut_setup (void);
void cut_teardown (void);

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

    inv_index = NULL;
    fixed_index = NULL;
    phrase = NULL;
    fplist = NULL;
}

void
cut_teardown (void)
{
    inv_index_free(sample_inv_index);

    if (inv_index)
        inv_index_free(inv_index);
    if (fixed_index)
        fixed_index_free(fixed_index);
    if (phrase)
        phrase_free(phrase);
}

void
test_fixed_index_new_empty (void)
{
    inv_index = inv_index_new();
    fixed_index = fixed_index_new(inv_index);
    cut_assert_not_null(fixed_index);
    cut_assert_equal_uint(0, fixed_index_numterms(fixed_index));
}

void
test_fixed_index_new (void)
{
    fixed_index = fixed_index_new(sample_inv_index);
    cut_assert_equal_uint(4, fixed_index_numterms(fixed_index));
}

void
test_fixed_index_get (void)
{
    inv_index = inv_index_new();
    fixed_index = fixed_index_new(inv_index);

    cut_assert_null(fixed_index_get(fixed_index, term1));

    fixed_index_free(fixed_index);
    fixed_index = fixed_index_new(sample_inv_index);
    cut_assert_not_null(take_fplist(fixed_index_get(fixed_index, term1)));

    fplist = take_fplist(fixed_index_get(fixed_index, term1));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 0));

    fplist = take_fplist(fixed_index_get(fixed_index, term2));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 1));

    fplist = take_fplist(fixed_index_get(fixed_index, term3));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(3, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 2));

    fplist = take_fplist(fixed_index_get(fixed_index, term4));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 3));
    cut_assert_not_null(fixed_posting_list_check(fplist, 3, 3));

    fplist = fixed_index_get(fixed_index, term5);
    cut_assert_null(fplist);
}

void
test_fixed_index_phrase_get (void)
{
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
    phrase = phrase_new();
    phrase_append(phrase, "this");

    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));
    fixed_posting_list_free(fplist);

    phrase_append(phrase, "is");
    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));
    fixed_posting_list_free(fplist);

    phrase_append(phrase, "a");
    fplist = fixed_index_phrase_get(fixed_index, phrase);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 0));
    fixed_posting_list_free(fplist);
}

void
test_fixed_index_load_dump_empty (void)
{
    inv_index = inv_index_new();
    fixed_index = fixed_index_new(inv_index);
    fixed_index_dump(fixed_index, "/tmp/fixed_index.dump");
    fixed_index_free(fixed_index);

    fixed_index = fixed_index_load("/tmp/fixed_index.dump");
    cut_assert_not_null(fixed_index);
    cut_assert_equal_uint(0, fixed_index_numterms(fixed_index));
}

void
test_fixed_index_load_dump (void)
{
    fixed_index = fixed_index_new(sample_inv_index);

    fixed_index_dump(fixed_index, "/tmp/fixed_index.dump");
    fixed_index_free(fixed_index);
    fixed_index = fixed_index_load("/tmp/fixed_index.dump");

    fplist = take_fplist(fixed_index_get(fixed_index, term1));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 0));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 0));

    fplist = take_fplist(fixed_index_get(fixed_index, term2));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 1));

    fplist = take_fplist(fixed_index_get(fixed_index, term3));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(3, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 0, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 1, 2));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 2));

    fplist = take_fplist(fixed_index_get(fixed_index, term4));
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(2, fixed_posting_list_size(fplist));
    cut_assert_not_null(fixed_posting_list_check(fplist, 2, 3));
    cut_assert_not_null(fixed_posting_list_check(fplist, 3, 3));

    fplist = take_fplist(fixed_index_get(fixed_index, term5));
    cut_assert_null(fplist);
}
