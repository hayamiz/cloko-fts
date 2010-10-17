
#include "test.h"

void test_bloom_filter_key_hash (void);
void test_bloom_filter_new (void);
void test_bloom_filter_free (void);
void test_bloom_filter_insert (void);
void test_bloom_filter_check (void);

static gint
int_compare(const guint *a, const guint *b)
{
    if (*a < *b)
        return -1;
    else if (*a > *b)
        return 1;
    else
        return 0;
}
void
test_bloom_filter_key_hash (void)
{
    guint i, j, k;
    guint coll = 0;
    guint size = 10000;
    guint *nums = g_malloc(sizeof(guint) * size * 10);
    for(i = 0;i < size;i++){
        for(j = 0;j < 10;j++){
            guint hash = bloom_filter_key_hash(j, i);
            // if (i < 10){
            //     g_print("hash(%d,%d):	%u\n",
            //             i, j, hash);
            // }
            nums[i * 10 + j] = hash;
        }
    }
    for(i = 0;i < size;i++){
        qsort(nums + 10*i, 10, sizeof(guint),
              (int (*)(const void*, const void*)) int_compare);
    }
    for(i = 0;i < size - 1;i++){
        for(k = i+1;k < size;k++){
            for(j = 0;j < 10;j++){
                if (nums[10*i+j] != nums[10*k+j]){
                    goto no_collision;
                }
            }
            coll++;
        no_collision:
            continue;
        }
    }

    g_print("collision: %d in %d\n", coll, size);
    cut_assert(coll < size / 5000);
}

void
test_bloom_filter_new (void)
{
    BloomFilter *filter;
    gdouble error_rate = 0.1;
    guint capacity = 10;

    filter = bloom_filter_new(g_int_hash, error_rate, capacity);
    cut_assert_not_null(filter);
    cut_assert_equal_uint(capacity, bloom_filter_capacity(filter));
    cut_assert_equal_double(error_rate, bloom_filter_error_rate(filter), 0.001);
    cut_assert(0 < bloom_filter_bitsize(filter));

    cut_assert_not_null(cut_take(bloom_filter_new(NULL, error_rate, capacity),
                                 bloom_filter_free));
    cut_assert_null(bloom_filter_new(g_int_hash, -0.1, 0));
    cut_assert_null(bloom_filter_new(g_int_hash, 1.2, 0));
}

void
test_bloom_filter_free (void)
{
    BloomFilter *filter;
    filter = bloom_filter_new(g_int_hash, 0.01, 100);
    bloom_filter_free(filter);
}

void
test_bloom_filter_insert (void)
{
    BloomFilter *filter;
    filter = bloom_filter_new(g_int_hash, 0.01, 10);

    gint i;
    for(i = 0; i < 100; i++){
        bloom_filter_insert(filter, 1);
    }
    cut_assert_equal_uint(100, bloom_filter_numelems(filter));
}

void
test_bloom_filter_check (void)
{
    BloomFilter *filter;
    guint size;
    guint i;
    gdouble error_rate = 0.01;

    size = 10000;
    filter = bloom_filter_new(g_int_hash, error_rate, size);

    for(i = 0; i < size * 10; i++){
        if (size % 10 == 0){
            bloom_filter_insert(filter, i);
        }
    }
    for(i = 0; i < size * 10; i++){
        if (size % 10 == 0){
            cut_assert(bloom_filter_check(filter, i) == TRUE);
        }
    }
    gint false_positive;
    false_positive = 0;
    for(i = 0; i < size * 10; i++){
        if (size % 10 != 0){
            if (bloom_filter_check(filter, i) == TRUE){
                false_positive ++;
            }
        }
    }
    g_print("false_positive: %d\n", false_positive);
    cut_assert(false_positive <= size * error_rate);
}
