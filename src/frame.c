
#include "inv-index.h"

#define IOV_MAX 1024

static __thread struct iovec iov[IOV_MAX];
static __thread Frame local_frame;

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

    for (idx = 0; idx < document_set_size(docset); idx++){
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
    sz = send(sockfd, raw_frame->buf, sizeof(Frame), 0);
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
    Frame *frame;
    RawFrame *raw_frame;
    gssize sz;
    guint32 result_num;
    struct iovec *loop_iov;

    if (frames == NULL) {
        bzero(&local_frame, sizeof(Frame));
        local_frame.type = FRM_RESULT;
        local_frame.extra_field = 0;
        local_frame.content_length = 0;
        local_frame.body.q.length = 0;

        return frame_send(sockfd, &local_frame);
    }

    frm_length = g_list_length(frames);
    loop_iov = iov;

    if (frm_length == 1){
        g_return_val_if_fail(frame_type(frame) == FRM_RESULT, -1);
        return frame_send(sockfd, frames->data);
    }

    result_num = 0;
    for(frame_cell = frames; frame_cell != NULL; frame_cell = frame_cell->next){
        frame = frame_cell->data;
        result_num += frame->extra_field;
    }

    // make the first frame
    frame = frames->data;
    frame->extra_field = result_num;
    switch (frame->type){
    case FRM_RESULT:
        frame->type = (guint32) FRM_LONG_RESULT_FIRST;
        g_return_val_if_fail(frame->content_length == frame->body.r.length, -1);
        frame->content_length = frame->body.r.length - 1; // cut off trailing null-char
        frame->body.lrf.frm_length = frm_length;
        break;
    case FRM_LONG_RESULT_FIRST:
        frame->body.lrf.frm_length = frm_length;
        break;
    default:
        break;
    }

    frame_cell = frames->next;;
    for (frm_rest = frm_length - 1; frm_rest > 0; frm_rest -= IOV_MAX) {
        loop_frm_length = (frm_rest > IOV_MAX ? IOV_MAX : frm_rest);
        for(frm_idx = 0; frm_idx < loop_frm_length; frm_idx++, frame_cell = frame_cell->next){
            g_return_val_if_fail(frame->type == FRM_RESULT ||
                                 frame->type == FRM_LONG_RESULT_FIRST ||
                                 frame->type == FRM_LONG_RESULT_REST,
                                 -1);
            frame = frame_cell->data;
            raw_frame = (RawFrame *) frame;
            frame->extra_field = result_num;
            switch (frame->type){
            case FRM_RESULT:
                g_return_val_if_fail(frame->content_length == frame->body.r.length, -1);
                frame->content_length--; // cut off trailing null-char
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
            iov[frm_idx].iov_base = raw_frame->buf;
            iov[frm_idx].iov_len = sizeof(Frame);
        }
        sz = writev(sockfd, iov, loop_frm_length);
        g_return_val_if_fail(sz == sizeof(Frame) * loop_frm_length, -1);
        loop_iov += loop_frm_length;
    }
}

gssize  frame_send_query         (gint sockfd, const gchar *query)
{
    bzero(&local_frame, sizeof(Frame));
    g_return_val_if_fail(strlen(query) <= FRAME_CONTENT_SIZE, -1);
    local_frame.type = FRM_QUERY;
    local_frame.content_length = strlen(query);
    local_frame.body.q.length = local_frame.content_length;
    memcpy(&local_frame.body.q.buf, query, local_frame.content_length);

    return frame_send(sockfd, &local_frame);
}

gssize  frame_send_bye         (gint sockfd)
{
    bzero(&local_frame, sizeof(Frame));
    local_frame.type = FRM_BYE;
    local_frame.content_length = 0;

    return frame_send(sockfd, &local_frame);
}

gssize  frame_recv               (gint sockfd, Frame *frame)
{
    RawFrame *raw_frame;
    gssize sz;

    raw_frame = (RawFrame *) frame;
    sz = recv(sockfd, raw_frame->buf, FRAME_SIZE, 0);
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

    frames = NULL;

    frame = frame_new();
    sz = frame_recv(sockfd, frame);
    if (sz == 0) {
        return NULL;
    }
    g_return_val_if_fail(sz == FRAME_SIZE, NULL);

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
    for (frm_rest = frm_length - 1; frm_rest > 0; frm_rest -= IOV_MAX) {
        loop_frm_length = (frm_rest > IOV_MAX ? IOV_MAX : frm_rest);
        for (idx = 0; idx < loop_frm_length; idx++){
            frame = frame_new();
            raw_frame = (RawFrame *) frame;
            iov[idx].iov_base = raw_frame->buf;
            iov[idx].iov_len = FRAME_SIZE;
            last_frame = g_list_append(last_frame, frame);
            last_frame = g_list_last(last_frame);
        }
        sz = readv(sockfd, iov, loop_frm_length);
        g_return_val_if_fail(sz == FRAME_SIZE * loop_frm_length, NULL);
    }

    for (frame_cell = frames->next; frame_cell != NULL; frame_cell = frame_cell->next){
        frame = frame_cell->data;
        g_return_val_if_fail(frame->type == FRM_LONG_RESULT_REST, NULL);
        g_return_val_if_fail(frame->content_length == frame->body.lrr.length, NULL);
    }

    return frames;
}

guint32
frame_type (Frame *frame)
{
    return (guint32)frame->type;
}


