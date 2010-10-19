// -*- coding: utf-8 -*-

#include "test.h"

static const gchar *docset001_path;

// general-purpose variables
static Document *doc;
static DocumentSet *docset;
static FixedIndex *findex;
static FixedPostingList *fplist;

void cut_setup (void)
{
    const gchar *dir = g_getenv("BASE_DIR");

    dir = dir ? dir : ".";
    cut_set_fixture_data_dir(dir, "fixtures", NULL);

    docset001_path = cut_take_string(cut_build_fixture_data_path("docset001.txt", NULL));

    doc = NULL;
    docset = NULL;
}

void cut_teardown (void)
{
    if (doc)
        document_free(doc);
}

typedef struct _termentry {
    guint num;
    const gchar *term;
} termentry_t;

void
test_search_docset001 (void)
{
    GList *list;
    const FixedPostingList *fplist;
    termentry_t *entry;
    // ./src/termdump -u -c 500 -C < test/fixtures/docset001.txt|sort|uniq > test/test-search-terms.inc
    termentry_t termentries[] =
    {
        #include "test-search-terms.inc"

        {0, NULL}};

    docset = document_set_load(docset001_path, 0);
    findex = document_set_make_fixed_index(docset, 0);

    cut_assert_not_null(findex);
    cut_assert_equal_uint(1609, fixed_index_numterms(findex)); // use termdump to count it

    for(entry = termentries; entry->term != NULL; entry++){
        fplist = take_fplist(fixed_index_get(findex, entry->term));
        if (!fplist) {
            fprintf(stderr, "\nterm search failed: \"%s\"\n", entry->term);
            cut_assert_not_null(fplist);
        }
        if (entry->num != fixed_posting_list_size(fplist)){
            fprintf(stderr, "\nterm search failed: \"%s\"\n", entry->term);
            cut_assert_equal_uint(entry->num, fixed_posting_list_size(fplist));
        }
    }

    const gchar *phrase_strs[] =
        {"今年こそはゴルフを極めようと",
         "今年こそはゴルフで",
         // "流では大変な回り道になります。「理にかなった",
         "自己流では大変な回り道になります。「理にかなった",
         // "時に限ってやって来るんですねもう久しぶりに同級",
         "に限ってやって来るんですねもう久しぶりに",
         "日程前から",
         "２",
         "配信中。ぜひ、",
         "ＢＡＴＴＬＥ　",
         "ＡＣＥＳ−」の",
         "はまぐりは対となる貝殻としか組み合わせることができないので、",
         "貝紅山形県",
         // "彩",
         "彩色",
         "鶴岡市松岡機業）、",
         "予約しちゃいましたとも。",
         "２３日の",
         "タイム",
         "６０時間" ,
         "ｎａｍｅ　’＊．ｈｔｍｌ’　｜　ｘａｒｇｓ　ｐｅｒｌ　−ｐ　−ｉ　−",
         "ｆｉｎｄ　．−",
         "４年ほど前に作った，静的な",
         // "気温５度って…明日も寒いんやろ？寒いと意味もなくイライラするからあかん",
         "気温５度って…明日も寒いんやろ？寒いと意味もなくイライラするからあか",
         "手の",
         "殺人的や最高",
         "アルトネリコ",
         "フィンネルノーマルエンドクリア。",
         "総勢６名で、肉を平らげます（笑）万両のお",
         "万両」へ新年",
         "万両へ行く",
         "分の感想です。",

         NULL};
    const gchar *phrase_str;

    guint idx;
    for (idx = 0; phrase_strs[idx] != NULL; idx++){
        phrase_str = phrase_strs[idx];
        if (!take_fplist(
                fixed_index_phrase_get(findex, take_phrase(phrase_from_string(phrase_str))))){
            fprintf(stderr, "\nphrase search failed:\n %s\n", phrase_str);
            Phrase *phrase = take_phrase(phrase_from_string(phrase_str));
            guint j;
            fprintf(stderr, "  Prase:\n");
            for (j = 0; j < phrase_size(phrase); j++){
                fprintf(stderr, "    \"%s\"\n", phrase_nth(phrase, j));
            }
            fixed_index_phrase_get(findex, take_phrase(phrase_from_string(phrase_str)));
            cut_test_fail("phrase search failed.");
        }

        // fprintf(stderr, "phrase search ok: [%d] %s\n", idx, phrase_str);
    }



    list = gcut_list_new(
        take_phrase_new("今年こそはゴルフで"),
        take_phrase_new("自己流では大変な回り道になります。「理にかなった"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));

    list = gcut_list_new(
        take_phrase_new("自己流では大変な回り道になります。「理にかなった"),
        take_phrase_new("今年こそはゴルフで"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));

    list = gcut_list_new(
        take_phrase_new("流では大変な回り道になります。「理にかなった"),
        take_phrase_new("今年こそはゴルフで"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    cut_assert_null(fplist);

    list = gcut_list_new(
        take_phrase_new("予約しちゃいましたとも。"),
        take_phrase_new("２３日の"),
        take_phrase_new("タイム"),
        take_phrase_new("６０時間" ),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));

    list = gcut_list_new(
        take_phrase_new("アルトネリコ"),
        take_phrase_new("フィンネルノーマルエンドクリア。"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));

    list = gcut_list_new(
        take_phrase_new("フィンネルノーマルエンドクリア。"),
        take_phrase_new("アルトネリコ"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(1, fixed_posting_list_size(fplist));

    list = gcut_list_new(
        take_phrase_new("分の感想です。"),
        NULL);
    fplist = fixed_index_multiphrase_get(findex, list);
    take_fplist((FixedPostingList *) fplist);
    cut_assert_not_null(fplist);
    cut_assert_equal_uint(5, fixed_posting_list_size(fplist));

    fplist = NULL;
}
