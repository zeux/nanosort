/**
 * nanosort
 *
 * Copyright (C) 2021, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https://github.com/zeux/nanosort
 *
 * This library is distributed under the MIT License. See notice at the end of
 * this file.
 *
 * Thank you to Andrei Alexandrescu for his branchless Lomuto partition code and
 * Gerben Stavenga for further research of branchless partitions; their work
 * inspired this algorithm.
 */
#pragma once

#include <assert.h>
#include <stddef.h>

// TODO: These are profiling helpers, remove before release
#ifdef _MSC_VER
#define NANOSORT_INLINE __forceinline
#define NANOSORT_NOINLINE __declspec(noinline)
#else
#define NANOSORT_INLINE __attribute__((always_inline))
#define NANOSORT_NOINLINE __attribute__((noinline))
#endif

namespace nanosort_detail {

struct Less {
  template <typename T>
  bool operator()(const T& l, const T& r) const {
    return l < r;
  }
};

template <typename It>
struct IteratorTraits {
  typedef typename It::value_type value_type;
};

template <typename T>
struct IteratorTraits<T*> {
  typedef T value_type;
};

template <typename T>
NANOSORT_INLINE void swap(T& l, T& r) {
#if __cplusplus >= 201103L
  T t(static_cast<T&&>(l));
  l = static_cast<T&&>(r);
  r = static_cast<T&&>(t);
#else
  T t(l);
  l = r;
  r = t;
#endif
}

// Return median of 5 elements in the array
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE T median5(It first, It last, Compare comp) {
  size_t n = last - first;
  assert(n >= 5);

  T e0 = first[(n >> 2) * 0];
  T e1 = first[(n >> 2) * 1];
  T e2 = first[(n >> 2) * 2];
  T e3 = first[(n >> 2) * 3];
  T e4 = first[n - 1];

  // 5-element median network
  if (comp(e1, e0)) swap(e1, e0);
  if (comp(e4, e3)) swap(e4, e3);
  if (comp(e3, e0)) swap(e3, e0);

  if (comp(e1, e4)) swap(e1, e4);
  if (comp(e2, e1)) swap(e2, e1);
  if (comp(e3, e2)) swap(e2, e3);

  if (comp(e2, e1)) swap(e2, e1);

  return e2;
}

// Split array into x<pivot and x>=pivot
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE It partition(T pivot, It first, It last, Compare comp) {
  It res = first;
  for (It it = first; it != last; ++it) {
    bool r = comp(*it, pivot);
    swap(*res, *it);
    res += r;
  }
  return res;
}

// Splits array into x<=pivot and x>pivot
template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE It partition_rev(T pivot, It first, It last, Compare comp) {
  It res = first;
  for (It it = first; it != last; ++it) {
    bool r = comp(pivot, *it);
    swap(*res, *it);
    res += !r;
  }
  return res;
}

// Push root down through the heap
template <typename It, typename Compare>
void sift_heap(It heap, size_t count, size_t root, Compare comp) {
  assert(count > 0);
  size_t last = (count - 1) >> 1;

  while (root < last) {
    assert(root * 2 + 2 < count);

    size_t next = root;
    next = comp(heap[next], heap[root * 2 + 1]) ? root * 2 + 1 : next;
    next = comp(heap[next], heap[root * 2 + 2]) ? root * 2 + 2 : next;

    if (next == root) break;
    swap(heap[root], heap[next]);
    root = next;
  }

  if (root == last && root * 2 + 1 < count &&
      comp(heap[root], heap[root * 2 + 1])) {
    swap(heap[root], heap[root * 2 + 1]);
  }
}

// Sort array using heap sort
template <typename It, typename Compare>
NANOSORT_NOINLINE void sort_heap(It first, It last, Compare comp) {
  if (first == last) return;

  It heap = first;
  size_t count = last - first;

  for (size_t i = count / 2; i > 0; --i) {
    sift_heap(heap, count, i - 1, comp);
  }

  for (size_t i = count - 1; i > 0; --i) {
    swap(heap[0], heap[i]);
    sift_heap(heap, i, 0, comp);
  }
}

// BubbleSort works better it has N(N-1)/2 stores, but x is updated in the
// inner loop. This is cmp/cmov sequence making the inner loop 2 cycles.
// TODO: auto, moves
template <typename It, typename Compare>
NANOSORT_NOINLINE void BubbleSort(It first, It last, Compare comp) {
  auto n = last - first;
  for (auto i = n; i > 1; i--) {
    auto x = first[0];
    for (decltype(n) j = 1; j < i; j++) {
      auto y = first[j];
      bool is_smaller = comp(y, x);
      first[j - 1] = is_smaller ? y : x;
      x = is_smaller ? x : y;
    }
    first[i - 1] = x;
  }
}

// BubbleSort2 bubbles two elements at a time. This means it's doing N(N+1)/4
// iterations and therefore much less stores. Correctly ordering the cmov's it
// is still possible to execute the inner loop in 2 cycles with respect to
// data dependencies. So in effect this cuts running time by 2x, even though
// it's not cutting number of comparisons.
// TODO: auto, moves
template <typename It, typename Compare>
NANOSORT_NOINLINE void BubbleSort2(It first, It last, Compare comp) {
  auto n = last - first;
  for (auto i = n; i > 1; i -= 2) {
    auto x = first[0];
    auto y = first[1];
    if (comp(y, x)) swap(y, x);
    for (decltype(n) j = 2; j < i; j++) {
      auto z = first[j];
      bool is_smaller = comp(z, y);
      auto w = is_smaller ? z : y;
      y = is_smaller ? y : z;
      is_smaller = comp(z, x);
      first[j - 2] = is_smaller ? z : x;
      x = is_smaller ? x : w;
    }
    first[i - 2] = x;
    first[i - 1] = y;
  }
}

template <typename It, typename Compare>
NANOSORT_NOINLINE void SelectionSort(It first, It last, Compare comp) {
  size_t n = last - first;
  if (n <= 1) return;

  for (size_t i = 0; i < n - 1; i++) {
    size_t k = i;
    for (size_t j = i + 1; j < n; j++) {
      k = comp(first[j], first[k]) ? j : k;
    }
    swap(first[i], first[k]);
  }
}

template <typename It, typename Compare>
NANOSORT_NOINLINE void CocktailSort(It first, It last, Compare comp) {
  size_t n = last - first;
  if (n <= 1) return;
  auto arr = &*first;
  for (size_t i = 0, j = n - 1; i < j; i++, j--) {
    size_t min = i, max = i;
    for (size_t k = i + 1; k <= j; k++) {
      max = comp(arr[max], arr[k]) ? k : max;
      min = comp(arr[k], arr[min]) ? k : min;
    }

    // shifting the min.
    swap(arr[i], arr[min]);

    // Shifting the max. The equal condition
    // happens if we shifted the max to arr[min_i]
    // in the previous swap.
    swap(arr[j], arr[i == max ? min : max]);
  }
}

template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE void InsertionSort(It begin, It end, Compare comp) {
  if (begin == end) return;

  for (It it = begin + 1; it != end; ++it) {
    T val = *it;
    It hole = it;
    while (hole > begin && comp(val, *(hole - 1))) {
      *hole = *(hole - 1);
      hole--;
    }
    *hole = val;
  }
}

template <typename It, typename Compare>
NANOSORT_NOINLINE void GnomeSort(It begin, It end, Compare comp) {
  It pos = begin;
  while (pos != end) {
    if (pos == begin || !comp(*pos, *(pos - 1)))
      pos++;
    else {
      swap(*pos, *(pos - 1));
      pos--;
    }
  }
}

template <typename It, typename Compare>
NANOSORT_NOINLINE void BubbleSortW(It first, It last, Compare comp) {
  size_t n = last - first;
  while (n > 1) {
    size_t newn = 0;
    for (size_t i = 1; i < n; ++i) {
      if (comp(first[i], first[i - 1])) {
        swap(first[i], first[i - 1]);
        newn = i;
      }
    }
    n = newn;
  }
}

template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE void NetworkSort(It first, It last, Compare comp) {
#define NANOSORT_PAIR(i, j) \
  if (comp(e##i, e##j)) swap(e##i, e##j)

  size_t n = last - first;

  T e0, e1, e2, e3;

  switch (n) {
    case 4:
      e0 = first[0], e1 = first[1], e2 = first[2], e3 = first[3];
      NANOSORT_PAIR(0, 1);
      NANOSORT_PAIR(2, 3);
      NANOSORT_PAIR(0, 2);
      NANOSORT_PAIR(1, 3);
      NANOSORT_PAIR(1, 2);
      first[0] = e0, first[1] = e1, first[2] = e2, first[3] = e3;
      break;

    case 3:
      e0 = first[0], e1 = first[1], e2 = first[2];
      NANOSORT_PAIR(0, 1);
      NANOSORT_PAIR(0, 2);
      NANOSORT_PAIR(1, 2);
      first[0] = e0, first[1] = e1, first[2] = e2;
      break;

    case 2:
      e0 = first[0], e1 = first[1];
      NANOSORT_PAIR(0, 1);
      first[0] = e0, first[1] = e1;
      break;

    default:;
  }

#undef NANOSORT_PAIR
}

template <typename T, typename It, typename Compare>
NANOSORT_NOINLINE void sort(It first, It last, size_t limit, Compare comp) {
  const size_t kSmallSortThreshold = 16;

  for (;;) {
    if (last - first <= kSmallSortThreshold) {
      // TODO: evaluate alternatives
      BubbleSort2(first, last, comp);
      return;
    }

    if (limit == 0) {
      nanosort_detail::sort_heap(first, last, comp);
      return;
    }

    T pivot = median5<T>(first, last, comp);
    It mid = partition(pivot, first, last, comp);

    // For skewed partitions compute new midpoint by separating equal elements
    bool skew = mid - first <= (last - first) >> 3;
    It midr = skew ? partition_rev(pivot, mid, last, comp) : mid;

    // Per MSVC STL, this allows 1.5 log2(N) recursive steps
    limit = (limit >> 1) + (limit >> 2);

    if (mid - first <= last - midr) {
      sort<T>(first, mid, limit, comp);
      first = midr;
    } else {
      sort<T>(midr, last, limit, comp);
      last = mid;
    }
  }
}

}  // namespace nanosort_detail

template <typename It, typename Compare>
NANOSORT_NOINLINE void nanosort(It first, It last, Compare comp) {
  typedef typename nanosort_detail::IteratorTraits<It>::value_type T;
  nanosort_detail::sort<T>(first, last, last - first, comp);
}

template <typename It>
NANOSORT_NOINLINE void nanosort(It first, It last) {
  typedef typename nanosort_detail::IteratorTraits<It>::value_type T;
  nanosort_detail::sort<T>(first, last, last - first, nanosort_detail::Less());
}

/**
 * Copyright (c) 2021 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
