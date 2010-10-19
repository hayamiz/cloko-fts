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
    const FixedPostingList *fplist;
    termentry_t *entry;
    // ./src/termdump -q < test/fixtures/docset001.txt|awk '{print $1}'|sort|uniq -c|sort -rn|head -20|awk '{print "{"$1","$2"},"}'
    termentry_t termentries[] =
    {
        {83,"の"},
        {64,"　"},
        {61,"。"},
        {49,"に"},
        {49,"、"},
        {41,"を"},
        {40,"て"},
        {36,"０"},
        {34,"た"},
        {33,"は"},
        {32,"！"},
        {25,"で"},
        {25,"し"},
        {24,"です"},
        {24,"１"},
        {24,"」"},
        {24,"「"},
        {23,"２"},
        {23,"．"},
        {22,"が"},
        {1,"»"},
        {1,"¹´"},
        {1,"!　"},
        {1,"(　"},
        {1,"-"},
        {1,"2010"},
        {1,"30"},
        {1,"60"},
        {1,"Bible"},
        {1,"CM"},
        {1,"DELUX"},
        {1,"GyaO"},
        {1,"HTML"},
        {1,"Silent"},
        {1,"["},
        {1,"]["},
        {1,"]　"},
        {1,"memo"},
        {1,"perl"},
        {1,"　)"},
        {1,"｜"},
        {1,"『"},
        {1,"』"},
        {1,"＊"},
        {1,"◆"},
        {1,"∀"},
        {1,"５月"},
        {1,"６月"},
        {1,"７"},
        {3,"新年"},
        {3,"新"},
        {3,"書き"},
        {3,"初"},
        {3,"修正"},
        {3,"私"},
        {3,"魂"},
        {3,"軌跡"},
        {3,"音楽"},
        {3,"ピンチ"},
        {3,"ビデオ"},
        {3,"パチスロ"},
        {3,"ゴルフ"},
        {3,"クリップ"},
        {3,"エヴァンゲリオン"},
        {3,"もの"},
        {3,"はま"},
        {3,"ので"},
        {3,"など"},
        {3,"として"},

        {0, NULL}};

    docset = document_set_load(docset001_path, 0);
    findex = document_set_make_fixed_index(docset);

    cut_assert_not_null(findex);
    cut_assert_equal_uint(675, fixed_index_numterms(findex)); // use termdump to count it

    for(entry = termentries; entry->term != NULL; entry++){
        fplist = take_fplist(fixed_index_get(findex, entry->term));
        cut_assert_not_null(fplist,
                            cut_message("search failed on \"%s\"\n", entry->term));
        cut_assert_equal_uint(entry->num, fixed_posting_list_size(fplist));
    }

    const gchar *phrase_strs[] =
        {"今年こそはゴルフを極めようと",
         "今年こそはゴルフで",
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

        fprintf(stderr, "phrase search ok: [%d] %s\n", idx, phrase_str);
    }
}
