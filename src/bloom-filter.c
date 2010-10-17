
#include "bloom-filter.h"

#define BF_KEY_MAX 100

enum { BITSPERWORD = 64, SHIFT = 6, MASK = 0x3F };

static int primes[] = {2, 79, 191, 311, 439, 577, 709, 857,
                       3, 83, 193, 313, 443, 587, 719, 859,
                       5, 89, 197, 317, 449, 593, 727, 863,
                       7, 97, 199, 331, 457, 599, 733, 877,
                       11, 101, 211, 337, 461, 601, 739, 881,
                       13, 103, 223, 347, 463, 607, 743, 883,
                       17, 107, 227, 349, 467, 613, 751, 887,
                       19, 109, 229, 353, 479, 617, 757, 907,
                       23, 113, 233, 359, 487, 619, 761, 911,
                       29, 127, 239, 367, 491, 631, 769, 919,
                       31, 131, 241, 373, 499, 641, 773, 929,
                       37, 137, 251, 379, 503, 643, 787, 937};

guint
bloom_filter_key_hash (guint salt, guint key)
{
    guint salt1 = primes[(salt*key) % BF_KEY_MAX];
    guint salt2 = primes[((salt+1)*key) % BF_KEY_MAX];
    guint salt3 = primes[((salt+2)*key) % BF_KEY_MAX];

    guint hash;
    hash = (key ^ 0xAAAAAAAA) ^
        (0x44444444 * salt1);
    hash = (key ^ 0xAAAAAAAA) ^
        (0x44444444 * salt2);
    hash = (salt3 | key ^ 0xAAAAAAAA) ^
        (0x44444444 * salt3);
    return ;
}

BloomFilter *
bloom_filter_new (BFHashFunc init_hash_func,
                  gdouble error_rate,
                  guint capacity)
{
    BloomFilter *filter;
    guint k, best_k;
    gdouble n, m, lowest_m;
    gdouble tmp;

    if (error_rate < 0 || error_rate > 1.0){
        return NULL;
    }

    filter = g_malloc(sizeof(BloomFilter));
    filter->error_rate = error_rate;
    filter->capacity = capacity;
    filter->init_hash_func = init_hash_func;
    filter->numelems = 0;

    best_k = 4;
    lowest_m = G_MAXUINT;
    n = capacity;
    for (k = 1; k < BF_KEY_MAX; k++) {
        tmp = 1 / (double)k;
        tmp = pow(error_rate, tmp);
        tmp = log(1.0 - tmp);
        tmp = 1.0/ tmp;
        tmp = -(double)k * (double)n / tmp;
        m = ceil(tmp);
        // m = ceil(- (double)k * (double)n / log(1.0 - pow(error_rate, 1 / (double)k)));
        if (m < lowest_m) {
            lowest_m = m;
            best_k = k;
        }
    }

    filter->bitsize = (guint) lowest_m;
    filter->keynum = best_k;
    if (filter->bitsize == 0) {
        g_free(filter);
        return NULL;
    }

    filter->vecsize = (filter->bitsize >> SHIFT) + 1;
    filter->vec = g_malloc(sizeof(guint64) * filter->vecsize);
    return filter;
}

void
bloom_filter_free        (BloomFilter *filter)
{
    if (!filter) return;
    g_free(filter->vec);
    g_free(filter);
}

guint
bloom_filter_capacity    (BloomFilter *filter)
{
    if (!filter) return 0;
    return filter->capacity;
}

guint
bloom_filter_numelems (BloomFilter *filter)
{
    if (!filter) return 0;
    return filter->numelems;
}

guint
bloom_filter_bitsize (BloomFilter *filter)
{
    if (!filter) return 0;
    return filter->bitsize;
}

guint
bloom_filter_vecsize (BloomFilter *filter)
{
    if (!filter) return 0;
    return filter->vecsize;
}

gdouble
bloom_filter_error_rate  (BloomFilter *filter)
{
    if (!filter) return 0.0;
    return filter->error_rate;
}

void
bloom_filter_insert (BloomFilter *filter, gint val)
{
    guint k;
    guint hash;

    if (!filter) return;

    for (k = 0;k < filter->keynum;k++){
        hash = bloom_filter_key_hash(k, val);
        filter->vec[(hash >> SHIFT) % filter->vecsize] |= 1 << (hash & MASK);
    }
    filter->numelems++;
}

gboolean
bloom_filter_check (BloomFilter *filter, gint val)
{
    guint k;
    guint hash;
    guint idx;

    if (!filter) return FALSE;

    for (k = 0;k < filter->keynum;k++){
        hash = bloom_filter_key_hash(k, val);
        idx = (hash % filter->bitsize) >> SHIFT;
        if (filter->vec[idx] & (1 << (hash & MASK)) == 0){
            return FALSE;
        }
    }

    return TRUE;
}


