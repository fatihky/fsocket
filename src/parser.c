#include "utils/debug.h"
#include "utils/cont.h"
#include "parser.h"
#include "types.h"
#include "frame.h"
#include "stream.h"

#ifdef FSOCKET_PARSER_DEBUG
#	define debug(...) printf(__VA_ARGS__)
#else
#	define debug(...)
#	undef rmy_log
#	define rmy_log(...)
#endif

void fsocket_parser_init(fsocket_parser_t *parser) {
	parser->must_read = 0;
	parser->state = FSOCK_PROT_START;
	fsocket_frame_init(&parser->frame);
}

void fsocket_parser_deinit(fsocket_parser_t *self) {
	fsocket_frame_deinit(&self->frame);
	fsocket_parser_init(self);
}

static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

static long long mstime(void) {
    return ustime()/1000;
}

void fsocket_parser_parse(fsocket_stream_t *self) {
	// parse incoming data
	rmy_log("parse incoming data buffer size: %zu ======", self->in_buffer->size);

	// not_parsed: how much data we have got to parse
	uint32_t not_parsed = 0;
	char *p = nitro_buffer_data(self->in_buffer, (int *)&not_parsed);
	rmy_log("not_parsed: %u", not_parsed);

	not_parsed -= self->in_buffer_cursor;
	rmy_log("not_parsed: %u self->in_buffer_cursor: %llu", not_parsed,
		(unsigned long long)self->in_buffer_cursor);
	p += self->in_buffer_cursor;

	if(self->parser.must_read > 0 && not_parsed <= self->parser.must_read)
		self->parser.must_read -= not_parsed;

	if(not_parsed < self->parser.must_read) {
		rmy_log("not_parsed(%u) < self->parser.must_read(%zu)", not_parsed,
			self->parser.must_read);
		return; // wait for more data
	}

	for(;;)
	{
		// this frame is initialized at parser initialization at socket creation
		// every time you are done with parser's frame, you must re-initialize it
		fsocket_frame_t *f = &self->parser.frame;
		rmy_log("for(;;) ---- %p", f->data);

		switch(self->parser.state)
		{
			case FSOCK_PROT_START:
			{
				rmy_log("FSOCK_PROT_START");
				// TODO: bu 4 de başlık boyutuna eşenmeli
				if(not_parsed < 4) { // wait for more data
					return; // FSTREAM_OK;
				}
				rmy_log("f->size: %u", f->size);
				memcpy(&f->size, p, 4);				// get current frame's size
				rmy_log("f->size: %u", f->size);
				f->data = malloc(f->size); // allocate some space for data
				not_parsed -= 4;							// subtract headers size from not parsed data size
				p += 4; 											// move pointer for 4 bytes(frame header size)
				self->parser.state = FSOCK_PROT_END; // change parser state
				self->in_buffer_cursor += 4;	// increment cursor value
			}

			case FSOCK_PROT_END:
			{
				rmy_log("FSOCK_PROT_END");

				if(not_parsed < f->size - f->cursor) {
					memcpy(f->data + f->cursor, p, not_parsed); 	 // copy current data to the frame
					f->cursor += not_parsed;											 // increment the cursor's value
					self->parser.must_read = f->size - not_parsed; // set needed data size
					nitro_buffer_destroy(self->in_buffer);				 // re-initialize buffer
					self->in_buffer = nitro_buffer_new();
					self->in_buffer_cursor = 0; // start from beginning
					f->type = FSOCKET_FRAME_PARTIAL;							 // set frame type
					//sdsrange(_prv->in_buf, sdslen(_prv->in_buf) - not_parsed, -1);
					rmy_log("not_parsed %u < f->size %u - f->cursor %u %u",
						not_parsed, f->size, f->cursor, f->size - f->cursor);
					return;																				 // wait for more data
				}

				// we have all remaining data of frame

				uint32_t not_parsed_tmp = not_parsed;
				uint32_t wanted = f->size;
				uint32_t tmp = wanted <= not_parsed ? wanted : not_parsed;

				rmy_log("f->data: %p f->cursor: %u", f->data, f->cursor);
				memcpy(f->data + f->cursor, p, tmp); 	 // copy current data to the frame
				f->type = FSOCKET_FRAME_FULL;				   // set frame type

				// create copy
				fsocket_frame_t *copy = fsocket_frame_new(f->size, f->data);
				rmy_log("TODO: push frame to the queue %.*s", f->size, (char *)f->data);
				// queue_push_right(self->in_frames, copy);
				copy->pipe = self->pipe;
				debug("[parser.c:%d] calling incref for: %p\n", __LINE__, self->pipe);
				fsocket_pipe_incref(self->pipe);
				queue_push_right(self->pipe->parent->in_frames, copy);
				rmy_log("[parser] in_frames: %p\n", self->pipe->parent->in_frames);

				// move pointer
				p += f->size;
				not_parsed -= f->size;

				// reset the parser
				fsocket_parser_deinit(&self->parser);

				rmy_log("not_parsed: %u", not_parsed);

				if (not_parsed == 0) {
					nitro_buffer_destroy(self->in_buffer);
					self->in_buffer = nitro_buffer_new();
					self->in_buffer_cursor = 0;
				}
				else {
					rmy_log("not_parsed(%u) then self->in_buffer_cursor(%llu) += not_parsed_tmp(%u)",
						not_parsed, (unsigned long long)self->in_buffer_cursor, not_parsed_tmp);
					self->in_buffer_cursor += not_parsed_tmp;
				}
			}
			default: break;
		}

		if(not_parsed == 0 || self->parser.must_read)
			break;
	}

	rmy_log("fsocket_parser_parse end ----");
}