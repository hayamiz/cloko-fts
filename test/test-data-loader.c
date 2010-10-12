#include <cutter.h>
#include <inv-index.h>

void cut_setup (void);
void test_load_data (void);


void cut_setup (void)
{
    const gchar *dir = g_getenv("BASE_DIR");
    dir = dir ? dir : ".";

    cut_set_fixture_data_dir(dir, "fixtures", NULL);
}


void
test_load_data (void)
{
    InvIndex *inv_index;
    const gchar *test_data_file = cut_build_fixture_data_path("test_data01.txt", NULL);

    g_printerr("file = %s\n", test_data_file);
}
