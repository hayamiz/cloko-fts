#ifndef TEST_H
#define TEST_H

#include <cutter.h>
#include <inv-index.h>
#include <bloom-filter.h>
#include <locale.h>

#define take_fplist(fplist)                                             \
    ((FixedPostingList *) cut_take((fplist),                            \
                                   (CutDestroyFunction) fixed_posting_list_free))

#define take_phrase(phrase) \
    (Phrase *) cut_take((phrase), (CutDestroyFunction) phrase_free)


#endif
