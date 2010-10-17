#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <glib.h>
#include <math.h>

typedef guint (*BFHashFunc) (gconstpointer key);

typedef struct _BloomFilter {
    guint vecsize; // in bit
    guint keynum;
    guint capacity;
    guint numelems;
    gdouble error_rate;
    BFHashFunc init_hash_func;
    guint64 *vec;
} BloomFilter;

guint        bloom_filter_key_hash    (guint salt, guint key);
BloomFilter *bloom_filter_new         (BFHashFunc hash_func,
                                       gdouble error_rate,
                                       guint capacity);
void         bloom_filter_free        (BloomFilter *filter);
guint        bloom_filter_capacity    (BloomFilter *filter);
gdouble      bloom_filter_error_rate  (BloomFilter *filter);
guint        bloom_filter_numelems    (BloomFilter *filter);
guint        bloom_filter_vecsize     (BloomFilter *filter);
void         bloom_filter_insert      (BloomFilter *filter, gint val);
gboolean     bloom_filter_check       (BloomFilter *filter, gint val);

#endif
