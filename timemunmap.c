
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// 2MB * 1000 = 2GB
#define TOTAL_PAGES 1000

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

int main(void) {
  int err, iterations;
  int page_size = sysconf(_SC_PAGESIZE);
  char* ptr[TOTAL_PAGES];
  struct timespec ts;
  long long unsigned start_ns, delta;

  // syscall
  start_ns = time_ns(&ts);
  iterations = 1000000;
  for (int i = 0; i < iterations; i++) {
    if (syscall(SYS_gettid) <= 1) {
      exit(2);
    }
  }
  delta = time_ns(&ts) - start_ns;
  printf("%i system calls in %lluns (%.1fns/syscall)\n",
         iterations, delta, (delta / (float) iterations));

  // map
  iterations = TOTAL_PAGES;
  start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; ++i) {
    ptr[i] = mmap (NULL, page_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1, 0);

    if (ptr[i] != NULL)
      ptr[i][page_size - 1] = 'X';
    else
      printf("Failed to create a page #%d\n", i);
  }
  delta = time_ns(&ts) - start_ns;
  printf("%i map calls in %lluns (%.1fns/map)\n",
         iterations, delta, (delta / (float) iterations));

  // unmap
  start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; ++i) {
    err = munmap(ptr[i], page_size);
    if (err != 0) {
      printf("Error: %s\n", strerror(errno));
      exit(2);
    }
  }
  delta = time_ns(&ts) - start_ns;
  printf("%i unmap calls in %lluns (%.1fns/unmap)\n",
         iterations, delta, (delta / (float) iterations));

  return 0;
}
