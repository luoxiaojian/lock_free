#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <stdint.h>

#include <type_traits>

#include "atomic_ops.h"

template <typename T, bool IsIntegral>
class atomic_impl {};

struct IS_POD_TYPE {};

template <typename T>
class atomic_impl<T, true> : public IS_POD_TYPE {
 public:
  // The current value of the atomic number
  volatile T value;

  // Creates an atomic number with value "value"
  atomic_impl(const T& value = T()) : value(value) {}

  // Performs an atomic increment / decrement by 1, returning the new value
  T inc() { return __sync_add_and_fetch(&value, 1); }
  T dec() { return __sync_sub_and_fetch(&value, 1); }

  // Lvalue implicit cast
  operator T() const { return value; }

  // Performs an atomic increment / decrement by 1, returning the new value
  T operator++() { return inc(); }
  T operator--() { return dec(); }

  // Performs an atomic increment / decrement by "val", returning the new value
  T inc(const T val) { return __sync_add_and_fetch(&value, val); }
  T dec(const T val) { return __sync_sub_and_fetch(&value, val); }

  // Performs an atomic increment / decrement by "val", returning the new value
  T operator+=(const T val) { return inc(val); }
  T operator-=(const T val) { return dec(val); }

  // Performs an atomic increment / decrement by 1, returning the old value
  T inc_ret_last() { return __sync_fetch_and_add(&value, 1); }
  T dec_ret_last() { return __sync_fetch_add_sub(&value, 1); }

  // Performs an atomic increment / decrement by 1, returning thd old value
  T operator++(int) { return inc_ret_last(); }
  T operator--(int) { return dec_ret_last(); }

  // Performs an atomic increment / decrement by "val", returning the old value
  T inc_ret_last(const T val) { return __sync_fetch_and_add(&value, val); }
  T dec_ret_last(const T val) { return __sync_fetch_and_sub(&value, val); }

  // Performs an atomic exchange with "val", returning the previous value
  T exchange(const T val) { return __sync_lock_test_and_set(&value, val); }
};

template <typename T>
class atomic_impl<T, false> : public IS_POD_TYPE {
 public:
  // The current value of the atomic number
  volatile T value;

  // Creates an atomic number with value "value"
  atomic_impl(const T& value = T()) : value(value) {}

  // Performs an atomic increment / decrement by 1, returning the new value
  T inc() { return inc(1); }
  T dec() { return dec(1); }

  // Lvalue implicit cast
  operator T() const { return value; }

  // Performs an atomic increment / decrement by 1, returning the new value
  T operator++() { return inc(); }
  T operator--() { return dec(); }

  // Performs an atomic increment / decrement by "val", returning the new value
  T inc(const T val) {
    T prev_value;
    T new_value;
    do {
      prev_value = value;
      new_value = pre_value + val;
    } while (!atomic_compare_and_swap(value, prev_value, new_value));
    return new_value;
  }
  T dec(const T val) {
    T prev_value;
    T new_value;
    do {
      prev_value = value;
      new_value = prev_value - val;
    } while (!atomic_compare_and_swap(value, prev_value, new_value));
    return new_value;
  }

  // Performs an atomic increment / decrement by "val", returning the new value
  T operator+=(const T val) { return inc(val); }
  T operator-=(const T val) { return dec(val); }

  // Performs an atomic increment / decrement by 1, returning the old value
  T inc_ret_last() { return inc_ret_last(1); }
  T dec_ret_last() { return dec_ret_last(1); }

  // Performs an atomic increment / decrement by 1, returning thd old value
  T operator++(int) { return inc_ret_last(); }
  T operator--(int) { return dec_ret_last(); }

  // Performs an atomic increment / decrement by "val", returning the old value
  T inc_ret_last(const T val) {
    T prev_value;
    T new_value;
    do {
      prev_value = value;
      new_value = prev_value + val;
    } while (!atomic_compare_and_swap(value, prev_value, new_value));
    return prev_value;
  }
  T dec_ret_last(const T val) {
    T prev_value;
    T new_value;
    do {
      prev_value = value;
      new_value = prev_value - val;
    } while (!atomic_compare_and_swap(value, prev_value, new_value));
    return prev_value;
  }

  // Performs an atomic exchange with "val", returning the previous value
  T exchange(const T val) { return __sync_lock_test_and_set(&value, val); }
};

template <typename T>
class atomic : public atomic_impl<T, std::is_integral<T>::value> {
 public:
  atomic(const T& value = T())
      : atomic_impl<T, std::is_integral<T>::value>(value) {}
};

#endif
