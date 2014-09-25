#include "fstream.h"
#include <string.h>

fstream_frame *rmy_frame_new()
{
    fstream_frame *f = zmalloc(sizeof(fstream_frame));
    memset(f, '\0', sizeof(fstream_frame));
    return f;
}

fstream *fstream_new()
{
    fstream *s = zmalloc(sizeof(fstream));
    s->buf = sdsempty();
    s->obuf = sdsempty();
    s->must_read = 0;
    s->cmd = rmy_frame_new();
    s->parse_state = FSTREAM_PROT_START;
    return s;
}

void fstream_free(fstream *s)
{
    sdsfree(s->buf);
    sdsfree(s->obuf);
    zfree(s->cmd);
    zfree(s);
}

size_t fstream_input_buffer_length(fstream *s)
{
    return sdslen(s->buf);
}

size_t fstream_must_read(fstream *s)
{
    return s->must_read;
}

void *fstream_prepare_to_read(fstream *s, int will_read)
{
	s->buf = sdsMakeRoomFor(s->buf, will_read);
	size_t buflen = fstream_input_buffer_length(s);
	return (void *)s->buf + buflen;
}

void fstream_data_readed(fstream *s, int readed)
{
    if(s->must_read > 0 && readed < s->must_read)
        s->must_read -= readed;

    sdsIncrLen(s->buf, readed);
}

void *fstream_prepare_to_write(fstream *s, int *len)
{
    if(sdslen(s->obuf) == 0)
        return NULL;
    *len = (int)sdslen(s->obuf);
    return (void *)s->obuf;
}

void fstream_data_written(fstream *s, int written)
{
    if(written == (int)sdslen(s->obuf))
        sdsclear(s->obuf);
    else
        sdsrange(s->obuf, 0, written);
}

size_t fstream_output_size(fstream *s)
{
    return sdslen(s->obuf);
}

void fstream_grow_output_buffer(fstream *s, int grow)
{
    s->obuf = sdsMakeRoomFor(s->obuf, grow);
}

void fstream_grow_input_buffer(fstream *s, int grow)
{
    s->buf = sdsMakeRoomFor(s->buf, grow);
}

fstream_err fstream_encode_data(fstream *s, void *data, int len)
{
    size_t avail = sdsavail(s->obuf);
    int raw_len = len + 4;
    if(avail < raw_len)
    {
        s->obuf = sdsMakeRoomFor(s->obuf, raw_len);
    }
    s->obuf = sdscatlen(s->obuf, &len, 4);
    s->obuf = sdscatlen(s->obuf, data, len);
    return FSTREAM_OK;
}

fstream_err fstream_decode_input(fstream *s, fstream_frame_handler fn, void *arg)
{
    size_t not_parsed = sdslen(s->buf);
    fstream_err res = FSTREAM_OK;
    if(not_parsed < s->must_read)
        return FSTREAM_OK; // wait for more data
    
    sds p = s->buf;

    for(;;)
    {
        fstream_frame *f = s->cmd;
        switch(s->parse_state)
        {
            case FSTREAM_PROT_START:
            {
                if(not_parsed < 4) // wait for more data
                {
                    return FSTREAM_OK;
                }
                memcpy(&f->size, p, 4);
                p += 4; // move pointer for 4 bytes
                not_parsed -= 4;
                s->parse_state = FSTREAM_PROT_END;
            }
            case FSTREAM_PROT_END:
            {
                if(not_parsed < f->size)
                {
                    s->must_read = f->size - not_parsed;
                    sdsrange(s->buf, sdslen(s->buf) - not_parsed, -1);
                    return FSTREAM_OK;
                }
                f->data = p;
                if(fn)
                    fn(f, arg);
                p += f->size;
                not_parsed -= f->size;
                s->parse_state = FSTREAM_PROT_START; // return to start

                s->must_read = 0;
                sdsrange(s->buf, sdslen(s->buf) - not_parsed, -1);

                p = s->buf;
            }
            default: break;
        }

        if(not_parsed == 0 || s->must_read)
            break;
    }

    return res;
}