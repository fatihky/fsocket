#include <ev.h>
#include "utils/zmalloc.h"
#include "types.h"
#include "ctx.h"

#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif

#ifdef FSOCKET_CTX_DEBUG
#	define debug(...) printf(__VA_ARGS__)
#else
#	define debug(...)
#endif

#define MAX_STACK_FRAMES 64

static void *stack_traces[MAX_STACK_FRAMES];

/* Resolve symbol name and source location given the path to the executable 
   and an address */
int addr2line(char const * const program_name, void const * const addr) {
  char addr2line_cmd[512] = {0};
 
  /* have addr2line map the address to the relent line in the code */
  #ifdef __APPLE__
    /* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr); 
  #else
    sprintf(addr2line_cmd,"addr2line -f -p -e %.256s %p", program_name, addr); 
  #endif
 
  /* This will print a nicely formatted string specifying the
     function and source line of the address */
  return system(addr2line_cmd);
}

void posix_print_stack_trace() {
  int i, trace_size = 0;
  char **messages = (char **)NULL;
 
  trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
  messages = backtrace_symbols(stack_traces, trace_size);
 
  /* skip the first couple stack frames (as they are this function and
     our handler) and also skip the last frame as it's (always?) junk. */
  // for (i = 3; i < (trace_size - 1); ++i)
  // we'll use this for now so you can see what's going on
  for (i = 0; i < trace_size; ++i) {
    if (addr2line("examples/basic", stack_traces[i]) != 0) {
      debug("  error determining line # for: %s\n", messages[i]);
    }
  }

  if (messages) { free(messages); } 
}

static void stack_trace() {
	void *callstack[256];
	int size = backtrace(callstack, sizeof(callstack)/sizeof(callstack[0]));
	char **strings = backtrace_symbols(callstack, size);
	int i = 0;
	for (; i < size; ++i)
		debug("%s\n", strings[i]);
	free(strings);
}

static void ctx_async_cb(EV_P_ ev_async *a, int revents) {
	(void)revents;

	fsocket_ctx_t *ctx = (fsocket_ctx_t *)a->data;

	if (ctx->routine != NULL)
		ctx->routine(NULL);

	// ev_async_send(EV_A_ a);
}

fsocket_ctx_t *fsocket_ctx_new() {
	fsocket_ctx_t *ctx = c_new(fsocket_ctx_t);

	ctx->loop = ev_loop_new(0);
	ctx->pipes = listCreate();
	ctx->activecnt = 0;
	ctx->async.data =  ctx;

	ev_async_init(&ctx->async, ctx_async_cb);
	ev_async_start(ctx->loop, &ctx->async);
	ev_async_send(ctx->loop, &ctx->async);

	return ctx;
}

void fsocket_ctx_destroy(fsocket_ctx_t *ctx) {
	ev_loop_destroy(ctx->loop);
	listRelease(ctx->pipes);
	zfree(ctx);
}

void fsocket_ctx_start_io(fsocket_ctx_t *ctx, ev_io *io) {
	ev_io_start(ctx->loop, io);
}

void fsocket_ctx_stop_io(fsocket_ctx_t *ctx, ev_io *io) {
	ev_io_stop(ctx->loop, io);
}

void fsocket_ctx_run(fsocket_ctx_t *ctx, int run_once) {
	ev_run(ctx->loop, run_once);
}

void fsocket_ctx_inc_active(fsocket_ctx_t *ctx, fsocket_pipe_t *p) {
	listNode *node = listSearchKey(ctx->pipes, p);
	if (!node)
		listAddNodeTail(ctx->pipes, p);
	ctx->activecnt += 1;
	debug("%s activecnt: %d | for: %p\n", "[ctx inc]", ctx->activecnt, p);
}

void fsocket_ctx_dec_active(fsocket_ctx_t *ctx, fsocket_pipe_t *p) {
	listNode *node = listSearchKey(ctx->pipes, p);
	if (!node) {
		debug("BULUNAMADI: %p\n", p);
		//stack_trace();
	}
	ctx->activecnt -= 1;
	debug("%s activecnt: %d | for: %p\n", "[ctx dec]", ctx->activecnt, p);
	if (ctx->activecnt == 0) {
		debug("durdurdum olm ya\n");
		ev_async_stop(ctx->loop, &ctx->async);
	}
}