// This file is part of nanosort library; see nanosort.hpp for license details
#include <assert.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "nanosort.hpp"

template <typename T, typename Compare = std::less<T> >
void test_sort(const std::vector<T>& a, Compare comp = std::less<T>()) {
  std::vector<T> ns = a;
  nanosort(ns.begin(), ns.end(), comp);

  assert(std::is_sorted(ns.begin(), ns.end(), comp));

  std::vector<T> hs = a;
  nanosort_detail::sort_heap(hs.begin(), hs.end(), comp);

  assert(std::is_sorted(hs.begin(), hs.end(), comp));

  std::vector<T> ss = a;
  std::stable_sort(ss.begin(), ss.end());
  std::stable_sort(ns.begin(), ns.end());
  std::stable_sort(hs.begin(), hs.end());

  assert(ss == ns);
  assert(ss == hs);
}

int main() {
  const size_t N = 1000;

  {
    std::vector<int> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = i;
    test_sort(A);
    test_sort(A, std::greater<int>());
  }

  {
    std::vector<int> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = N - i;
    test_sort(A);
    test_sort(A, std::greater<int>());
  }

  {
    std::vector<float> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = N - i;
    test_sort(A);
    test_sort(A, std::greater<float>());
  }

  {
    std::vector<unsigned int> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = i * 123456789;
    test_sort(A);
    test_sort(A, std::greater<unsigned int>());
  }

  {
    std::vector<unsigned int> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = 0;
    test_sort(A);
    test_sort(A, std::greater<unsigned int>());
  }

  {
    std::vector<unsigned int> A(N);
    for (size_t i = 0; i < N; ++i) A[i] = i % 16;
    test_sort(A);
    test_sort(A, std::greater<unsigned int>());
  }

  {
    std::vector<unsigned int> A;
    test_sort(A);
  }
}
