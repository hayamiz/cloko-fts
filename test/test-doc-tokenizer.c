/* -*- coding: utf-8 -*- */

#include "test.h"

static const gchar *sample_string1 = "これはペンです。";
static const gchar *sample_string2 = "これはペンです。こんにちは世界。";

void cut_setup (void);

void test_tokenizer_new  (void);
void test_tokenizer_new2 (void);
void test_tokenizer_next (void);


void
test_tokenizer_new (void)
{
    Tokenizer *tok;

    tok = tokenizer_new(sample_string1);
    cut_assert_not_null(tok);
}

void
test_tokenizer_new2 (void)
{
    Tokenizer *tok;

    tok = tokenizer_new2(sample_string2, strlen(sample_string1));
    cut_assert_not_null(tok);
}

void
test_tokenizer_next (void)
{
    Tokenizer *tok;

    tok = tokenizer_new(sample_string1);
    cut_assert_equal_string("これ", tokenizer_next(tok));
    cut_assert_equal_string("は"  , tokenizer_next(tok));
    cut_assert_equal_string("ペン", tokenizer_next(tok));
    cut_assert_equal_string("です", tokenizer_next(tok));
    cut_assert_equal_string("。"  , tokenizer_next(tok));
}
