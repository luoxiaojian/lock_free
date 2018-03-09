#define _GNU_SOURCE

#include <errno.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "atomic.h"
#include "lf_queue.h"

#define ENQUEUE_TO 1000000

struct Node {
  int val;
  struct Node *next;
};

lf_queue<Node> queue;
atomic<int> produce_alive;

pid_t gettid(void) { return syscall(__NR_gettid); }

void *producer_routine(void *arg) {
  int i;
  int proc_num = (int)(long)arg;
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(proc_num, &set);

  if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &set)) {
    perror("sched_setaffinity");
    return NULL;
  }
}

void *consumer_routine(void *arg) {}

int main() {
  int procs = 0;
  int i;
  pthread_t *thrs;

  procs = (int)sysconf(_SC_NPROCESSORS_ONLN);

  if (procs < 0) {
    perror("sysconf");
    return -1;
  }

  if (procs < 2) {
    printf("Only %d processes...", procs);
    return 0;
  }

  thrs = (pthread_t *)malloc(sizeof(pthread_t) * procs);

  if (thrs == NULL) {
    perror("malloc");
    return -1;
  }

  printf("Starting %d threads, with 1 consumer and %d producers...\n", procs, 1,
         procs - 1);

  bool flag = false;

  struct timeval t1, t2;

  t1 = gettimeofday(&t1, NULL);

  produce_alive = procs - 1;

  for (i = 0; i < procs - 1; i++) {
    if (pthread_create(&thrs[i], NULL, producer_routine, (void *)(long)i)) {
      perror("pthread_create");
      procs = i;
      flag = true;
      produce_alive.dec();
      break;
    }
  }

  if (!flag) {
    if (pthread_create(&thrs[procs - 1], NULL, consumer_routine,
                       (void *)(long)(procs - 1))) {
      perror("pthread_create");
      procs = procs - 1;
    }
  }

  for (i = 0; i < procs; i++) {
    pthread_join(thrs[i], NULL);
  }

  t2 = gettimeofday(&t2, NULL);

  free(thrs);

  printf("After doing all the enqueue and dequeue, global_int value is: %d\n",
         global_int);
  printf("Expected value is: %d\n", ENQUEUE_TO * procs);
  double dur = t2.tv_usec - t1.tv_usec;
  dur = dur / 1000000.0 + (t2.tv_sec - t1.tv_sec);
  printf("Elapsed time is: %lf\n", dur);

  return 0;
}
