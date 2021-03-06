
#include "inv-index.h"

#define IOV_MAX 1024

static __thread struct iovec iov[IOV_MAX];
static __thread Frame local_frame;


static inline ssize_t
write_all (gint sockfd, const gchar *msg, ssize_t size)
{
    ssize_t sz;
    ssize_t sent = 0;
    // g_printerr("write_all(%d, %p, %d)\n", sockfd, msg, size);
    while(sent < size){
        if ((sz = write(sockfd, msg, size - sent)) == -1) {
            g_printerr("write(2) returned -1: errno = %d\n", errno);
            return sz;
        } else if (sz == 0) {
            break;
        }
        sent += sz;
        msg += sz;
    }

    // g_printerr("write_all: return %d\n", sent);
    return sent;
}

static inline ssize_t
read_all (gint sockfd, gchar *res, ssize_t size)
{
    ssize_t sz;
    ssize_t recved = 0;
    // g_printerr("read_all(%d, %p, %d)\n", sockfd, res, size);
    while(recved < size){
        if ((sz = read(sockfd, res, size - recved)) == -1){
            g_printerr("read(2) returned -1: errno %d\n", errno);
            return sz;
        } else if (sz == 0){
            break;
        }
        recved += sz;
        res += sz;
    }
    // g_printerr("read_all: return %d\n", recved);

    return recved;
}


static gssize
writev_all(gint sockfd, struct iovec *iov, int cnt)
{
    gssize total;
    guint offset;
    gssize sz;
    // g_printerr("writev_all(%d, %p, %d)\n", sockfd, iov, cnt);

    total = 0;
    offset = 0;
    while (total < cnt * FRAME_SIZE) {
        sz = writev(sockfd, iov, (cnt - offset));
        if (sz == -1){
            g_printerr("writev(2) returned -1: errno %d\n", errno);
        }
        // g_return_val_if_fail(sz % FRAME_SIZE == 0, -1);
        total += sz;
        while(sz > 0){
            if (sz > iov[0].iov_len) {
                sz -= iov[0].iov_len;
                iov++;
                offset++;
            } else {
                iov[0].iov_base += sz;
                iov[0].iov_len -= sz;
                sz = 0;
            }
        }
    }

    // g_printerr("writev_all:return %d\n",total);

    return total;
}


static gssize
readv_all(gint sockfd, struct iovec *iov, int cnt)
{
    gssize total;
    guint offset;
    // guint inner_offset;
    gssize sz;
    // g_printerr("readv_all(%d, %p, %d)\n", sockfd, iov, cnt);

    total = 0;
    offset = 0;
    while (total < cnt * FRAME_SIZE) {
        sz = readv(sockfd, iov + offset, (cnt - offset));
        if (sz == -1){
            g_printerr("readv(2) returned -1: errno %d\n", errno);
        }
        // g_return_val_if_fail(sz % FRAME_SIZE == 0, -1);
        total += sz;
        while(sz > 0){
            if (sz > iov[0].iov_len) {
                sz -= iov[0].iov_len;
                iov++;
                offset++;
            } else {
                iov[0].iov_base += sz;
                iov[0].iov_len -= sz;
                sz = 0;
            }
        }
    }

    // g_printerr("readv_all:return %d\n", total);

    return total;
}

Frame *
frame_new               (void)
{
    Frame *frame;
    frame = g_malloc(sizeof(Frame));
    bzero(frame, sizeof(Frame));

    return frame;
}

void
frame_free               (Frame *frame)
{
    g_free(frame);
}

void
frame_make_quit          (Frame *frame)
{
    bzero(frame, sizeof(Frame));
    frame->type = FRM_QUIT;
}

GList *
frame_make_result_from_fixed_posting_list (DocumentSet *docset,
                                           FixedPostingList *fplist)
{
    // extra_field に結果の数を代入
    gchar *raw_record;
    gchar *ptr;
    Frame *frame;
    GString *output;
    guint idx;
    gint rest_content_len;
    GList *frames;
    GList *last_frame;
    guint32 result_num;
    guint doc_id;
    guint32 frame_len;

    output = g_string_new("");

    result_num = fixed_posting_list_size(fplist);

    if (result_num == 0) {
        return NULL;
    }

    for (idx = 0; idx < fixed_posting_list_size(fplist); idx++){
        doc_id = fplist->pairs[idx].doc_id;
        raw_record = document_raw_record(document_set_nth(docset, doc_id));
        g_string_append(output, raw_record);
        g_free(raw_record);
    }

    rest_content_len = output->len;
    ptr = output->str;
    if (rest_content_len <= FRAME_CONTENT_SIZE) {
        frame = frame_new();
        frame->type = FRM_RESULT;
        frame->extra_field = result_num;
        frame->content_length = rest_content_len;
        frame->body.r.length = rest_content_len;
        memcpy(frame->body.r.buf, ptr, rest_content_len);
        frames = g_list_append(NULL, frame);
    } else {
        frame = frame_new();
        frame->type = FRM_LONG_RESULT_FIRST;
        frame->extra_field = result_num;
        frame->content_length = FRAME_CONTENT_SIZE;
        frame->body.lrf.frm_length = (rest_content_len - 1) / FRAME_CONTENT_SIZE + 1;
        memcpy(frame->body.r.buf, ptr, FRAME_CONTENT_SIZE);
        last_frame = frames = g_list_append(NULL, frame);
        rest_content_len -= FRAME_CONTENT_SIZE;

        for (ptr = output->str + FRAME_CONTENT_SIZE;
             ptr < output->str + output->len;
             ptr += FRAME_CONTENT_SIZE, rest_content_len -= FRAME_CONTENT_SIZE) {
            frame_len = (rest_content_len > FRAME_CONTENT_SIZE ?
                         FRAME_CONTENT_SIZE : rest_content_len);
            frame = frame_new();
            frame->type = FRM_LONG_RESULT_REST;
            frame->extra_field = result_num;
            frame->content_length = frame_len;
            frame->body.r.length = frame_len;
            memcpy(frame->body.r.buf, ptr, frame_len);
            last_frame = g_list_append(last_frame, frame);
            last_frame = g_list_last(last_frame);
        }
    }

    g_string_free(output, TRUE);
    return frames;
}

gssize  frame_send               (gint sockfd, Frame *frame)
{
    gssize sz;
    RawFrame *raw_frame;
    raw_frame = (RawFrame *) frame;
    sz = write_all(sockfd, raw_frame->buf, sizeof(Frame));
    g_return_val_if_fail(sz == sizeof(Frame), -1);
    return sz;
}

gssize  frame_send_multi_results (gint sockfd, GList *frames)
{
    guint32 frm_length;
    GList *frame_cell;
    gint frm_rest;
    guint loop_frm_length;
    guint frm_idx;
    guint frm_offset;
    Frame *frame;
    RawFrame *raw_frame;
    gssize sz;
    guint32 result_num;
    gssize total;

    if (frames == NULL) {
        bzero(&local_frame, sizeof(Frame));
        local_frame.type = FRM_RESULT;
        local_frame.extra_field = 0;
        local_frame.content_length = 0;
        local_frame.body.q.length = 0;

        return frame_send(sockfd, &local_frame);
    }

    frm_length = g_list_length(frames);
    frame = frames->data;

    if (frm_length == 1){
        g_return_val_if_fail(frame_type(frame) == FRM_RESULT, -1);
        return frame_send(sockfd, frames->data);
    }

    result_num = 0;
    for(frame_cell = frames; frame_cell != NULL; frame_cell = frame_cell->next){
        frame = frame_cell->data;
        if (frame->type == FRM_RESULT || frame->type == FRM_LONG_RESULT_FIRST)
            result_num += frame->extra_field;
    }

    // make the first frame
    frame = frames->data;
    frame->extra_field = result_num;
    switch (frame->type){
    case FRM_RESULT:
        frame->type = (guint32) FRM_LONG_RESULT_FIRST;
        g_return_val_if_fail(frame->content_length == frame->body.r.length, -1);
        frame->content_length = frame->body.r.length;
        frame->body.lrf.frm_length = frm_length;
        break;
    case FRM_LONG_RESULT_FIRST:
        frame->body.lrf.frm_length = frm_length;
        break;
    default:
        break;
    }

    // setup frames
    for (frame_cell = frames->next; frame_cell != NULL; frame_cell = frame_cell->next) {
        frame = frame_cell->data;
        g_return_val_if_fail(frame->type == FRM_RESULT ||
                             frame->type == FRM_LONG_RESULT_FIRST ||
                             frame->type == FRM_LONG_RESULT_REST,
                             -1);
        raw_frame = (RawFrame *) frame;
        frame->extra_field = result_num;
        switch (frame->type){
        case FRM_RESULT:
            frame->type = FRM_LONG_RESULT_REST;
            g_return_val_if_fail(frame->content_length == frame->body.r.length, -1);
            frame->content_length;
            frame->body.lrr.length = frame->content_length;
            break;
        case FRM_LONG_RESULT_FIRST:
            frame->type = FRM_LONG_RESULT_REST;
            frame->body.lrr.length = frame->content_length;
            break;
        case FRM_LONG_RESULT_REST:
            g_return_val_if_fail(frame->content_length == frame->body.r.length, -1);
            break;
        }
    }

    total = 0;
    for (frame_cell = frames, frm_idx = 0; frm_idx < frm_length;
         frm_idx++, frame_cell = frame_cell->next){
        frame = frame_cell->data;
        raw_frame = (RawFrame *) frame;
        sz = write_all(sockfd, raw_frame->buf, FRAME_SIZE);
        g_return_val_if_fail(sz >= 0, -1);
        total += sz;
        // iov[frm_idx].iov_base = raw_frame->buf;
        // iov[frm_idx].iov_len = FRAME_SIZE;
    }

    // g_printerr("frame_send_multi_resutls: total = %d\n", total);
    g_return_val_if_fail(total == FRAME_SIZE * frm_length, -1);

    return total;
}

gssize  frame_send_query         (gint sockfd, const gchar *query)
{
    Frame *frame;
    gssize sz;
    g_return_val_if_fail(strlen(query) <= FRAME_CONTENT_SIZE, -1);
    frame = frame_new();
    bzero(frame, sizeof(Frame));
    frame->type = FRM_QUERY;
    frame->content_length = strlen(query);
    frame->body.q.length = frame->content_length;
    memcpy(&frame->body.q.buf, query, frame->content_length);
    sz = frame_send(sockfd, frame);
    frame_free(frame);
    return sz;
}

gssize  frame_send_bye         (gint sockfd)
{
    Frame *frame;
    gssize sz;
    frame = frame_new();
    bzero(frame, sizeof(Frame));
    frame->type = FRM_BYE;
    frame->content_length = 0;
    sz = frame_send(sockfd, frame);
    frame_free(frame);
    return sz;
}

gssize  frame_send_quit         (gint sockfd)
{
    bzero(&local_frame, sizeof(Frame));
    local_frame.type = FRM_QUIT;
    local_frame.content_length = 0;

    return frame_send(sockfd, &local_frame);
}

gssize  frame_recv               (gint sockfd, Frame *frame)
{
    RawFrame *raw_frame;
    gssize sz;

    raw_frame = (RawFrame *) frame;
    sz = read_all(sockfd, raw_frame->buf, FRAME_SIZE);
    if (sz == 0) {
        return sz;
    }
    g_return_val_if_fail(sz == FRAME_SIZE, -1);

    return sz;
}

GList  *frame_recv_result        (gint sockfd)
{
    GList *frames;
    GList *frame_cell;
    GList *last_frame;
    Frame *frame;
    RawFrame *raw_frame;
    gssize sz;
    guint idx;
    guint32 frm_length;
    gint frm_rest;
    guint32 loop_frm_length;
    guint32 result_num;
    guint total;

    frames = NULL;

    total = 0;
    frame = frame_new();
    sz = frame_recv(sockfd, frame);
    if (sz == 0) {
        return NULL;
    }
    g_return_val_if_fail(sz == FRAME_SIZE, NULL);
    total += sz;

    g_return_val_if_fail(frame->type == FRM_RESULT ||
                         frame->type == FRM_LONG_RESULT_FIRST,
                         NULL);
    switch(frame->type){
    case FRM_RESULT:
        if (frame->extra_field == 0){
            g_return_val_if_fail(frame->content_length == 0, NULL);
            g_return_val_if_fail(frame->body.q.length == 0, NULL);
        }
        return g_list_append(NULL, frame);
        break;
    case FRM_LONG_RESULT_FIRST:
        frm_length = frame->body.lrf.frm_length;
        result_num = frame->extra_field;
        break;
    }
    frames = g_list_append(frames, frame);

    last_frame = frames;
    for (idx = 0; idx < frm_length - 1; idx++){
        frame = frame_new();
        raw_frame = (RawFrame *) frame;
        sz = read_all(sockfd, raw_frame->buf, FRAME_SIZE);
        total += sz;
        g_return_val_if_fail(sz == FRAME_SIZE, NULL);
        // iov[idx].iov_base = raw_frame->buf;
        // iov[idx].iov_len = FRAME_SIZE;
        last_frame = g_list_append(last_frame, frame);
        last_frame = g_list_last(last_frame);
    }
    // g_printerr("frame_recv_result: total = %d\n", total);
    g_return_val_if_fail(total == FRAME_SIZE * frm_length, NULL);

    // check content
    if (g_list_length(frames) == 1){
        frame = frames->data;
        g_return_val_if_fail(frame->type == FRM_RESULT, NULL);
    } else {
        for (frame_cell = frames->next; frame_cell != NULL; frame_cell = frame_cell->next){
            frame = frame_cell->data;
            g_return_val_if_fail(frame->type == FRM_LONG_RESULT_REST, NULL);
            g_return_val_if_fail(frame->content_length == frame->body.lrr.length, NULL);
        }
    }

    return frames;
}

guint32
frame_type (Frame *frame)
{
    return (guint32)frame->type;
}


