#include <sys/time.h>
#include "fsocket.h"
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

void on_connect(fsock_t *self, fsock_t *conn)
{
    FSOCK_LOG("client connected to server");
}

void on_data(fsock_t *self, fsock_t *conn, fsock_frame_t *f)
{
    received += 1;
    if(received == NUM_REQS)
    {
        long long ms = mstime() - start;
        FSOCK_LOG("%d requests takes %lld ms", NUM_REQS, ms);
    }
}

void on_disconnect(fsock_t *self, fsock_t *conn)
{
    FSOCK_LOG("server disconnected");
}

int main(int argc, char *argv[])
{
    EV_P = ev_loop_new(0);

    fsock_t *cli = fsock_connect(EV_A_ "127.0.0.1", 9123);
    fsock_on_conn(cli, on_connect);
    fsock_on_frame(cli, on_data);
    fsock_on_disconnect(cli, on_disconnect);

    if(argc == 2 && atoi(argv[1]) != -1)
    {
        NUM_REQS = atoi(argv[1]);
    } else if (argc != 2) {
        FSOCK_LOG("argc: %d", argc);
    } else {
        FSOCK_LOG("argv[1]: %s | atoi(%s): %d", argv[1], argv[1], atoi(argv[1]));
    }

    int i;
    for(i = 0; i < NUM_REQS; i++)
    {
        fsock_send(cli, "ping", 4);
    }

    start = mstime();
    ev_run(EV_A_ 0);
    return 0;
}