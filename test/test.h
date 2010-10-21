#ifndef TEST_H
#define TEST_H

#include <cutter.h>
#include <locale.h>
#include <gcutter.h>

#include <inv-index.h>
#include <bloom-filter.h>
#include <thread-pool.h>
#include <skiplist.h>

#define take_fplist(fplist)                                             \
    ((FixedPostingList *) cut_take((fplist),                            \
                                   (CutDestroyFunction) fixed_posting_list_free))

#define take_phrase_new(str) \
    (Phrase *) cut_take(phrase_from_string(str), (CutDestroyFunction) phrase_free)

#define take_phrase(phrase) \
    (Phrase *) cut_take((phrase), (CutDestroyFunction) phrase_free)


#endif
