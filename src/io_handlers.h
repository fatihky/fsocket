#ifndef _FSOCKET_IO_HANDLERS_H
#define _FSOCKET_IO_HANDLERS_H

void io_accept(EV_P_ struct ev_io *a, int revents);
void io_read(EV_P_ struct ev_io *r, int revents);
void io_write(EV_P_ struct ev_io *w, int revents);

#endif