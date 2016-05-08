#pragma once

#include "../utils/thread.h"

struct fsock_thread *f_choose_thr();
void fsock_workers_lock();
void fsock_workers_unlock();
void fsock_workers_signal();
