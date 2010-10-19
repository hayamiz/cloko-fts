
#include "test.h"

static Frame *frame;
static RawFrame *raw_frame;

void
cut_setup (void)
{
    frame = NULL;
    raw_frame = NULL;
}

void
cut_teardown (void)
{
    if (frame)
        frame_free(frame);
    if (raw_frame)
        frame_free((Frame *)raw_frame);
}

void
test_frame_new (void)
{
    cut_assert_equal_uint(sizeof(Frame), sizeof(RawFrame));
    frame = frame_new();
    cut_assert_not_null(frame);
    raw_frame = (RawFrame *) frame;

    cut_assert_equal_pointer(raw_frame, raw_frame->buf);

    raw_frame = NULL;
}
