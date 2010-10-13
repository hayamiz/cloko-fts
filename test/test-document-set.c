// -*- coding: utf-8 -*-

#include <cutter.h>
#include <inv-index.h>

static const gchar *test_data_file;

void cut_setup (void);

void test_document_parse (void);

void test_document_set_load (void);


void cut_setup (void)
{
    const gchar *dir = g_getenv("BASE_DIR");
    dir = dir ? dir : ".";

    cut_set_fixture_data_dir(dir, "fixtures", NULL);

    test_data_file = cut_build_fixture_data_path("test_load_data.txt", NULL);
}

void
test_document_parse (void)
{
    Document *doc;
    const gchar *str = cut_get_fixture_data_string(test_data_file, NULL);
    const gchar *endptr;
    guint offset;
    guint doc_id;

    doc_id = 0;

    offset = 1;
    doc = document_parse(str, offset, doc_id, &endptr);
    cut_assert_null(doc);

    offset = 0;
    doc = document_parse(str, offset, doc_id, &endptr);
    cut_assert_equal_int(0, document_id(doc));
    cut_assert_equal_uint(0, document_position(doc));
    cut_assert(str + document_position(doc), document_body_pointer(doc));
    cut_assert_equal_string("きょうの判決（1月19日・横浜地裁）", document_title(doc));
    cut_assert_equal_uint(1263957666, document_time(doc));
    cut_assert_equal_string("http://0-3459.at.webry.info/201001/article_7.html",
                            document_url(doc));
    cut_assert_equal_string("# http://0-3459.at.webry.info http://0-3459.at.webry.info/201001/article_7.html きょうの判決（1月19日・横浜地裁） 1263957666\n・・・この日は４０５号法廷で今年初の裁判員裁判が、４０４号法廷では学園理事長が起こした自動車運転過失致死事件の被告人質問がそれぞれ行われていた関係もあってか、結構多くの傍聴人が裁判所に訪れていた。\nEOD\n",
                            cut_take_string(document_raw_record(doc)));
    cut_assert_equal_uint(294, document_body_size(doc));

    doc_id ++;

    doc = document_parse(str, endptr - str, doc_id, &endptr);
    cut_assert_equal_int(1, document_id(doc));
    cut_assert_equal_uint(439, document_position(doc));
    cut_assert(str + document_position(doc), document_body_pointer(doc));
    cut_assert_equal_string("きょうの判決（1月7日・横浜地裁）", document_title(doc));
    cut_assert_equal_uint(1263180592, document_time(doc));
    cut_assert_equal_string("http://0-3459.at.webry.info/201001/article_2.html",
                            document_url(doc));
    cut_assert_equal_string("# http://0-3459.at.webry.info http://0-3459.at.webry.info/201001/article_2.html きょうの判決（1月7日・横浜地裁） 1263180592\n・・・この日の午前中は、以下の判決の他に傷害事件の審理もあり、証人尋問が行われた。\n当該事件の被告は捜査段階での供述を公判で一部翻したようなのだが、その割には当の被告に緊張感の欠片も見られなかった。\nEOD\n",
                            cut_take_string(document_raw_record(doc)));
    cut_assert_equal_uint(295, document_body_size(doc));
}

void
test_document_set_load (void)
{
    DocumentSet *docset;
    Document    *doc;
    const gchar *docset_buf;
    InvIndex *inv_index;
    // const gchar **doc1_terms = g_strsplit(cut_get_fixture_data_string("test_load_data_doc1_terms.txt", NULL),
    //                                       "\n",
    //                                       58);

    gint i = 0;
    docset = document_set_load(test_data_file);
    cut_assert_not_null(docset);
    cut_assert_equal_uint(2, document_set_size(docset));
    docset_buf = document_set_buffer(docset);
    cut_assert_not_null(docset_buf);

    doc = document_set_nth(docset, 0);
    cut_assert_equal_int(0, document_id(doc));
    cut_assert_equal_uint(0, document_position(doc));
    cut_assert(docset_buf + document_position(doc), document_body_pointer(doc));
    cut_assert_equal_uint(294, document_body_size(doc));
    cut_assert_equal_string("きょうの判決（1月19日・横浜地裁）", document_title(doc));
    cut_assert_equal_uint(1263957666, document_time(doc));
    cut_assert_equal_string("http://0-3459.at.webry.info/201001/article_7.html",
                            document_url(doc));
    cut_assert_equal_string("# http://0-3459.at.webry.info http://0-3459.at.webry.info/201001/article_7.html きょうの判決（1月19日・横浜地裁） 1263957666\n・・・この日は４０５号法廷で今年初の裁判員裁判が、４０４号法廷では学園理事長が起こした自動車運転過失致死事件の被告人質問がそれぞれ行われていた関係もあってか、結構多くの傍聴人が裁判所に訪れていた。\nEOD\n",
                            cut_take_string(document_raw_record(doc)));

    doc = document_set_nth(docset, 1);
    cut_assert_equal_int(1, document_id(doc));
    cut_assert_equal_uint(439, document_position(doc));
    cut_assert(docset_buf + document_position(doc), document_body_pointer(doc));
    cut_assert_equal_uint(295, document_body_size(doc));
    cut_assert_equal_string("きょうの判決（1月7日・横浜地裁）", document_title(doc));
    cut_assert_equal_uint(1263180592, document_time(doc));
    cut_assert_equal_string("http://0-3459.at.webry.info/201001/article_2.html",
                            document_url(doc));
    cut_assert_equal_string("# http://0-3459.at.webry.info http://0-3459.at.webry.info/201001/article_2.html きょうの判決（1月7日・横浜地裁） 1263180592\n・・・この日の午前中は、以下の判決の他に傷害事件の審理もあり、証人尋問が行われた。\n当該事件の被告は捜査段階での供述を公判で一部翻したようなのだが、その割には当の被告に緊張感の欠片も見られなかった。\nEOD\n",
                            cut_take_string(document_raw_record(doc)));
}
