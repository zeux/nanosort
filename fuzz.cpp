// This file is part of nanosort library; see nanosort.hpp for license details
#include "nanosort.hpp"

#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <functional>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  typedef uint16_t T;

  const T* elements = reinterpret_cast<const T*>(Data);
  size_t count = Size / sizeof(T);

  std::vector<T> ss(elements, elements + count);
  std::vector<T> ns(elements, elements + count);
  std::vector<T> hs(elements, elements + count);

  std::sort(ss.begin(), ss.end());
  nanosort(ns.begin(), ns.end());
  nanosort_detail::heap_sort(hs.begin(), hs.end(), std::less<T>());

  assert(ss == ns);
  assert(ss == hs);
  return 0;
}
