#include "fsocket/fsocket.h"
#include "debug.h"

<<<<<<< HEAD
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

=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
void on_connect(fsock_cli *c, void *arg)
{
    log("client connected to %s:%d (fd: %d)", c->host, c->port, c->fd);
}

void on_data(fsock_cli *c, fstream_frame *f, void *arg)
{
<<<<<<< HEAD
    //log("server send: %s", f->data);
    received += 1;
    if(received == NUM_REQS)
    {
        long long ms = mstime() - start;
        log("%d requests takes %lld ms", NUM_REQS, ms);
    }
=======
    log("server send: %s", f->data);
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
}

void on_disconnect(fsock_cli *c, void *arg)
{
    log("server disconnected");
}

<<<<<<< HEAD
int main(int argc, char *argv[])
=======
int main()
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
{
    EV_P = ev_loop_new(0);

    fsock_cli *cli = fsock_cli_new(EV_A_ "127.0.0.1", 9123);
    fsock_cli_on_connect(cli, on_connect, NULL);
    fsock_cli_on_data(cli, on_data, NULL);
    fsock_cli_on_disconnect(cli, on_disconnect, NULL);

<<<<<<< HEAD
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
=======
    fsock_send(cli, "ping", 4);

>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
    ev_run(EV_A_ 0);
    return 0;
}