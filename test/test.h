#ifndef TEST_H
#define TEST_H

#include <cutter.h>
#include <inv-index.h>
#include <bloom-filter.h>

#define take_fplist(fplist)                                             \
    ((FixedPostingList *) cut_take((fplist),                            \
                                   (CutDestroyFunction) fixed_posting_list_free))


#endif
