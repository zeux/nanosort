// This file is part of nanosort library; see nanosort.hpp for license details
#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <vector>

#include "nanosort.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  typedef uint16_t T;

  const T* elements = reinterpret_cast<const T*>(Data);
  size_t count = Size / sizeof(T);

  std::vector<T> ss(elements, elements + count);
  std::vector<T> ns(elements, elements + count);

  std::sort(ss.begin(), ss.end());
  nanosort(ns.begin(), ns.end());

  assert(ss == ns);
  return 0;
}
