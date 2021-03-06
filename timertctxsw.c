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
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <linux/futex.h>

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

int main(void) {
  CPU_ZERO(&target_core);
  CPU_SET(1, &target_core);

  const int iterations = 5000000;
  struct timespec ts;
  const int shm_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
  const pid_t other = fork();
  int* futex = shmat(shm_id, NULL, 0);
  *futex = 0xA;
  if (other == 0) {
    pid_t tid = gettid();
    sched_setscheduler(tid, SCHED_FIFO, &RT_HIGH_PRIORITY);
    if(sched_setaffinity(tid, sizeof(cpu_set_t), &target_core)) {
        exit(-1);
    }
    usleep(500000);

    for (int i = 0; i < iterations; i++) {
      *futex = 0xB;
      sched_yield();
    }
    return 0;
  }

  pid_t tid = gettid();
  sched_setscheduler(tid, SCHED_FIFO, &RT_HIGH_PRIORITY);
  if(sched_setaffinity(tid, sizeof(cpu_set_t), &target_core)) {
      exit(-1);
  }
  usleep(500000);

  const long long unsigned start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; i++) {
    *futex = 0xA;
    sched_yield();
  }
  const long long unsigned delta = time_ns(&ts) - start_ns;

  const int nswitches = iterations * 2;
  printf("%i process with rt thread context switches in %lluns (%.1fns/ctxsw)\n",
         nswitches, delta, (delta / (float) nswitches));
  wait(futex);
  return 0;
}
