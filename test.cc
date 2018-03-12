#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

int global_int;

struct Node {
  int val;
  struct Node *next;
};

lf_queue<Node> que;
atomic<int> producer_alive;

pid_t gettid(void) { return syscall(__NR_gettid); }

struct Node *nodes;

void *producer_routine(void *arg) {
  int i;
  int proc_num = (int)(long)arg;
  struct Node *ptr = nodes + proc_num * ENQUEUE_TO;
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(proc_num, &set);

  if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &set)) {
    perror("sched_setaffinity");
    return NULL;
  }

  for (i = 0; i < ENQUEUE_TO; i++) {
    ptr->val = proc_num;
    ptr->next = NULL;
    que.enqueue(ptr);
    ptr++;
  }

  producer_alive.dec();

  return NULL;
}

void *consumer_routine(void *arg) {
  int i;
  int proc_num = (int)(long)arg;
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(proc_num, &set);

  if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &set)) {
    perror("sched_setaffinity");
    return NULL;
  }
  Node *ptr, *next;
  while (!que.empty() || producer_alive.value != 0) {
    ptr = que.dequeue_all();
    if (ptr == NULL) continue;
    while (!que.end_of_dequeue_list(ptr)) {
      global_int++;
      ptr = ptr->next;
    }
  }
  return NULL;
}

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
  nodes = (struct Node *)malloc(sizeof(Node) * ENQUEUE_TO * (procs - 1));

  if (thrs == NULL) {
    perror("malloc");
    return -1;
  }

  printf("Starting %d threads, with 1 consumer and %d producers...\n", procs,
         procs - 1);

  bool flag = false;

  struct timeval t1, t2;

  gettimeofday(&t1, NULL);

  producer_alive = procs - 1;
  global_int = 0;

  que.init();

  for (i = 0; i < procs - 1; i++) {
    if (pthread_create(&thrs[i], NULL, producer_routine, (void *)(long)i)) {
      perror("pthread_create");
      procs = i;
      flag = true;
      producer_alive.dec();
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

  gettimeofday(&t2, NULL);

  free(thrs);
  free(nodes);

  printf("After doing all the enqueue and dequeue, global_int value is: %d\n",
         global_int);
  printf("Expected value is: %d\n", ENQUEUE_TO * (procs - 1));
  double dur = t2.tv_usec - t1.tv_usec;
  dur = dur / 1000000.0 + (t2.tv_sec - t1.tv_sec);
  printf("Elapsed time is: %lf\n", dur);

  return 0;
}
