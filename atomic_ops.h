#ifndef ATOMIC_OPS_H_
#define ATOMIC_OPS_H_

#include <stdint.h>

/*
 * atomic instruction that is equivalent to the following:
 * if (a == oldval) {
 *   a = newval;
 *   return true;
 * } else {
 *   return false;
 * }
 */

template <typename T>
bool atomic_compare_and_swap(T& a, T oldval, T newval) {
  return __sync_bool_compare_and_swap(&a, oldval, newval);
}

template <typename T>
bool atomic_compare_and_swap(volatile T& a, T oldval, T newval) {
  return __sync_bool_compare_and_swap(&a, oldval, newval);
}

template <>
inline bool atomic_compare_and_swap(volatile double& a, double oldval,
                                    double newval) {
  volatile uint64_t* a_ptr = reinterpret_cast<volatile uint64_t*>(&a);
  const uint64_t* oldval_ptr = reinterpret_cast<const uint64_t*>(&oldval);
  const uint64_t* newval_ptr = reinterpret_cast<const uint64_t*>(&newval);
  return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
}

template <>
inline bool atomic_compare_and_swap(volatile float& a, float oldval,
                                    float newval) {
  volatile uint32_t* a_ptr = reinterpret_cast<volatile uint32_t*>(&a);
  const uint32_t* oldval_ptr = reinterpret_cast<const uint32_t*>(&oldval);
  const uint32_t* newval_ptr = reinterpret_cast<const uint32_t*>(&newval);
  return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
}

template <typename T>
void atomic_exchange(T& a, T& b) {
  b = __sync_lock_test_and_set(&a, b);
}

template <typename T>
void atomic_exchange(volatile T& a, T& b) {
  b = __sync_lock_test_and_set(&a, b);
}

/*
 * Set a to the newval, returning the old value
 */

template <typename T>
T fetch_and_store(T& a, const T& newval) {
  return __sync_lock_test_and_set(&a, newval);
}

#endif
