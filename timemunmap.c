
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// 4KB * 10000 = 40MB
#define TOTAL_PAGES 10000

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

int main(void) {
  int page_size = sysconf(_SC_PAGESIZE);
  char* ptr[TOTAL_PAGES];

  const int iterations = TOTAL_PAGES;
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

  int err;
  struct timespec ts;
  const long long unsigned start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; ++i) {
    err = munmap(ptr[i], page_size);
    if (err != 0) {
      printf("Error: %s\n", strerror(errno));
      exit(2);
    }
  }
  const long long unsigned delta = time_ns(&ts) - start_ns;
  printf("%i unmap calls in %lluns (%.1fns/unmap)\n",
         iterations, delta, (delta / (float) iterations));
  return 0;
}
