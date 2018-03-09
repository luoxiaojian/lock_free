
#ifndef LF_QUEUE_H_
#define LF_QUEUE_H_

#include "atomic.h"
#include "atomic_ops.h"

template <typename T>
class lf_queue {
 public:
  inline lf_queue() : sentinel(new T), head(sentinel), tail(sentinel) {
    sentinel->next = NULL;
  }

  static inline T *get_next(T *ptr) { return ptr->next; }

  static inline T **get_next_ptr(T *ptr) { return &(ptr->next); }

  ~lf_queue() { delete sentinel; }

  void enqueue(T *c) {
    // c->next = NULL
    // prev = c
    // swap(prev, tail)
    // prev->next = c
    (*get_next_ptr(c)) = NULL;
    T *prev = c;
    atomic_exchange(tail, prev);
    (*get_next_ptr(prev)) = c;
    numel.inc();
    asm volatile("" : : : "memory");
  }

  size_t approx_size() { return numel; }

  bool empty() const { return head->next == NULL; }

  T *dequeue_all() {
    // ret_head = head->next
    // sentinel->next = NULL
    // prev = sentinel
    // swap(prev, tail)
    // prev->next = sentinel
    // numel = 0
    T *ret_head = get_next(head);
    if (ret_head == NULL) return NULL;
    enqueue(sentinel);
    numel = 0;
    return ret_head;
  }

  T *end_of_dequeue_list() { return sentinel; }

  inline const bool end_of_dequeue_list(T *ptr) { return ptr == (sentinel); }

 private:
  atomic<size_t> numel;
  T *sentinel;
  T *head;
  T *tail;
};

#endif
