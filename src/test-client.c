#include "fsocket/fsocket.h"
#include "debug.h"

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

long long start = 0;
int received = 0;
int NUM_REQS = 1000;

void on_connect(fsock_cli *c, void *arg)
{
    log("client connected to %s:%d (fd: %d)", c->host, c->port, c->fd);
}

void on_data(fsock_cli *c, fstream_frame *f, void *arg)
{
    received += 1;
    if(received == NUM_REQS)
    {
        long long ms = mstime() - start;
        log("%d requests takes %lld ms", NUM_REQS, ms);
    }
}

void on_disconnect(fsock_cli *c, void *arg)
{
    log("server disconnected");
}

int main(int argc, char *argv[])
{
    EV_P = ev_loop_new(0);

    fsock_cli *cli = fsock_cli_new(EV_A_ "127.0.0.1", 9123);
    fsock_cli_on_connect(cli, on_connect, NULL);
    fsock_cli_on_data(cli, on_data, NULL);
    fsock_cli_on_disconnect(cli, on_disconnect, NULL);

    if(argc == 2 && atoi(argv[1]) != -1)
    {
        NUM_REQS = atoi(argv[1]);
    } else if (argc != 2) {
        log("argc: %d", argc);
    } else {
        log("argv[1]: %s | atoi(%s): %d", argv[1], argv[1], atoi(argv[1]));
    }

    int i;
    for(i = 0; i < NUM_REQS; i++)
    {
        fsock_send(cli, "ping", 4);
    }

    log("output length: %zu", fstream_output_size(cli->stream));

    start = mstime();
    ev_run(EV_A_ 0);
    return 0;
}