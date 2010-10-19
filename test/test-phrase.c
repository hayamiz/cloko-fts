
#include "test.h"

static Phrase *phrase;

void test_phrase_new (void);
void test_phrase_append (void);
void test_phrase_nth (void);


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

Phrase *
take_phrase(Phrase *phrase)
{
    return (Phrase *) cut_take(phrase, (CutDestroyFunction) phrase_free);
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
test_phrase_from_string (void)
{
    Phrase *phrase;

    cut_assert_null(phrase_from_string(NULL));

    phrase = take_phrase(phrase_from_string("今年こそはゴルフで"));
    cut_assert_equal_int(5, phrase_size(phrase));

    cut_assert_equal_string("今年", phrase_nth(phrase, 0));
    cut_assert_equal_string("こそ", phrase_nth(phrase, 1));
    cut_assert_equal_string("は", phrase_nth(phrase, 2));
    cut_assert_equal_string("ゴルフ", phrase_nth(phrase, 3));
    cut_assert_equal_string("で", phrase_nth(phrase, 4));
}
