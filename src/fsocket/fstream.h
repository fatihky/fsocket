#ifndef __FSTREAM_H
#define __FSTREAM_H

#include "sds.h"
#include "zmalloc.h"

typedef enum
{
    FSTREAM_PROT_START,
    FSTREAM_PROT_END
} fstream_prot_state;

typedef enum
{
    FSTREAM_OK,
    FSTREAM_ERR
} fstream_err;

typedef struct
{
    int size; // raw data size
    char *data;
} fstream_frame;

typedef struct {
	sds buf;
    size_t must_read; // must be readed
    fstream_frame *cmd; // last processing command
    fstream_prot_state parse_state;
    sds obuf;
} fstream;

typedef void (*fstream_frame_handler)(fstream_frame *f, void *arg);

// creates and initializes new stream
fstream *fstream_new();
void fstream_free(fstream *s);
size_t fstream_input_buffer_length(fstream *s); // get stream's input buffer's length
size_t fstream_must_read(fstream *s); // get how much data is needed to resume parsing
void *fstream_prepare_to_read(fstream *s, int will_read); // get writeable pointer
void fstream_data_readed(fstream *s, int readed); // update stream info
void *fstream_prepare_to_write(fstream *s, int *len); // prepare stream for writing data to other sources. returns pointer of start of not written data
void fstream_data_written(fstream *s, int written); // update stream info.
size_t fstream_output_size(fstream *s); // get how much data is stored in output bufffer
void fstream_grow_output_buffer(fstream *s, int grow); // grow output buffer
void fstream_grow_input_buffer(fstream *s, int grow); // grow input buffer's free space
fstream_err fstream_encode_data(fstream *s, void *data, int len); // encode data to stream
fstream_err fstream_decode_input(fstream *s, fstream_frame_handler handler, void *arg); // decode stream and iterate over frames

#endif