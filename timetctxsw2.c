// Copyright (C) 2010  Benoit Sigoure
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define gettid() syscall(SYS_gettid)

struct sched_param CFS_NORMAL_PRIORITY = { .sched_priority = 0, };
struct sched_param RT_HIGH_PRIORITY = { .sched_priority = 41, };
struct sched_param RT_LOW_PRIORITY = { .sched_priority = 40, };

cpu_set_t target_core;

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

static const int iterations = 5000000;

static void* thread(void*ctx) {
  pid_t tid = gettid();
  sched_setscheduler(tid, SCHED_FIFO, &RT_HIGH_PRIORITY);
  if(sched_setaffinity(tid, sizeof(cpu_set_t), &target_core)) {
      exit(-1);
  }
  usleep(500000);

  (void)ctx;
  for (int i = 0; i < iterations; i++)
      sched_yield();
  return NULL;
}

int main(void) {
  CPU_ZERO(&target_core);
  CPU_SET(1, &target_core);

  struct sched_param param;
  param.sched_priority = 1;
  if (sched_setscheduler(getpid(), SCHED_FIFO, &param))
    fprintf(stderr, "sched_setscheduler(): %s\n", strerror(errno));

  struct timespec ts;
  pthread_t thd;
  if (pthread_create(&thd, NULL, thread, NULL)) {
    return 1;
  }

  pid_t tid = gettid();
  sched_setscheduler(tid, SCHED_FIFO, &RT_HIGH_PRIORITY);
  if(sched_setaffinity(tid, sizeof(cpu_set_t), &target_core)) {
      exit(-1);
  }
  usleep(500000);

  long long unsigned start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; i++)
      sched_yield();
  long long unsigned delta = time_ns(&ts) - start_ns;

  const int nswitches = iterations * 2;
  printf("%i  thread context switches in %lluns (%.1fns/ctxsw)\n",
         nswitches, delta, (delta / (float) nswitches));
  return 0;
}
