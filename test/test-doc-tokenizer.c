/* -*- coding: utf-8 -*- */

#include "test.h"

static const gchar *sample_string1 = "これはペンです。";
static const gchar *sample_string2 = "これはペンです。こんにちは世界。";

void cut_setup (void);

void test_tokenizer_new  (void);
void test_tokenizer_new2 (void);
void test_tokenizer_next (void);
void test_tokenizer_renew (void);
void test_tokenizer_renew2 (void);

void
test_tokenizer_new (void)
{
    Tokenizer *tok;

    cut_assert_null(tokenizer_new(NULL));

    tok = tokenizer_new(sample_string1);
    cut_assert_not_null(tok);
    tokenizer_free(tok);
}

void
test_tokenizer_new2 (void)
{
    Tokenizer *tok;

    cut_assert_null(tokenizer_new2(NULL, 0));

    tok = tokenizer_new2(sample_string2, strlen(sample_string1));
    cut_assert_not_null(tok);
    tokenizer_free(tok);
}

void
test_tokenizer_next (void)
{
    Tokenizer *tok;

    tok = tokenizer_new(sample_string1);
    cut_assert_equal_string("これ", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("は"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("ペン", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("です", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("。"  , cut_take_string(tokenizer_next(tok)));
    tokenizer_free(tok);
}

void
test_tokenizer_renew (void)
{
    Tokenizer *tok;

    tok = tokenizer_new(sample_string1);
    cut_take_string(tokenizer_next(tok));
    cut_take_string(tokenizer_next(tok));
    cut_take_string(tokenizer_next(tok));

    tok = tokenizer_renew(tok, sample_string1);
    cut_assert_equal_string("これ", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("は"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("ペン", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("です", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("。"  , cut_take_string(tokenizer_next(tok)));
    tokenizer_free(tok);

    tok = tokenizer_renew(NULL, sample_string2);
    cut_assert_equal_string("これ", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("は"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("ペン", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("です", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("。"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("こんにちは"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("世界"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("。"  , cut_take_string(tokenizer_next(tok)));
    tokenizer_free(tok);
}

void
test_tokenizer_renew2 (void)
{
    Tokenizer *tok;

    cut_assert_null(tokenizer_new2(NULL, 0));

    tok = tokenizer_new2(sample_string2, strlen(sample_string1));
    cut_assert_not_null(tok);
    tok = tokenizer_renew2(tok, sample_string2, strlen(sample_string1));
    cut_assert_equal_string("これ", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("は"  , cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("ペン", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("です", cut_take_string(tokenizer_next(tok)));
    cut_assert_equal_string("。"  , cut_take_string(tokenizer_next(tok)));
    tokenizer_free(tok);
}
