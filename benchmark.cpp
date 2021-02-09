#include <cmath>
#include <string>
#include <vector>

#include "extern/hybrid_qsort.h"
#include "extern/pdqsort.h"
#include "nanosort.hpp"

const double kBenchRun = 0.1;

#if defined(__linux__)
double timestamp() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
}
#elif defined(_WIN32)
struct LARGE_INTEGER {
  __int64 QuadPart;
};
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency);

double timestamp() {
  LARGE_INTEGER freq, counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return double(counter.QuadPart) / double(freq.QuadPart);
}
#else
double timestamp() { return double(clock()) / double(CLOCKS_PER_SEC); }
#endif

typedef struct {
  uint64_t state;
  uint64_t inc;
} pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t *rng) {
  uint64_t oldstate = rng->state;
  // Advance internal state
  rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
  // Calculate output function (XSH RR), uses old state for max ILP
  uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  uint32_t rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

template <typename T, typename Sort>
void bench(const std::string &name, Sort sort, const std::vector<T> &data) {
  double divider = data.size() * log2(double(data.size()));

  std::vector<T> copy(data.size());

  double time = 0;
  double start = timestamp();

  while (timestamp() - start < kBenchRun) {
    copy = data;

    double ts0 = timestamp();
    sort(copy.begin(), copy.end());
    double ts1 = timestamp();

    if (ts1 - ts0 < time || time == 0) time = ts1 - ts0;
  }

  printf("%s: %.2f ns/op (%.2f ms total)\n", name.c_str(), time * 1e9 / divider,
         time * 1e3);
}

template <typename T>
void bench(const std::string &name, const std::vector<T> &data) {
  bench(
      name + ", std:sort", [](auto beg, auto end) { std::sort(beg, end); },
      data);
  bench(
      name + ", pdqsort ",
      [](auto beg, auto end) { pdqsort(beg, end); }, data);
  bench(
      name + ", expgrbns",
      [](auto beg, auto end) { exp_gerbens::QuickSort(beg, end); }, data);
  bench(
      name + ", nanosort", [](auto beg, auto end) { nanosort(beg, end); },
      data);
}

struct Pair {
  uint32_t key;
  uint32_t value;

  bool operator<(const Pair &other) const { return key < other.key; }
};

struct PairString {
  const char *key;
  uint32_t value;

  bool operator<(const PairString &other) const {
    return strcmp(key, other.key) < 0;
  }
};

int main() {
  pcg32_random_t rng = {42, 0};
  std::vector<uint32_t> test(1000000);

  for (size_t i = 0; i < test.size(); ++i) test[i] = pcg32_random_r(&rng);
  bench("random int", test);

  for (size_t i = 0; i < test.size(); ++i) test[i] = uint32_t(i);
  bench("sorted int", test);

  for (size_t i = 0; i < test.size(); ++i)
    test[i] = (i % 100 == 0) ? pcg32_random_r(&rng) : test[i - 1] + 1;
  bench("run100 int", test);

  for (size_t i = 0; i < test.size(); ++i) test[i] = uint32_t(test.size() - i);
  bench("sortre int", test);

  for (size_t i = 0; i < test.size(); ++i) test[i] = pcg32_random_r(&rng) % 1000;
  bench("eq1000 int", test);

  std::vector<Pair> test2(test.size());

  for (size_t i = 0; i < test.size(); ++i) test2[i].key = pcg32_random_r(&rng);
  bench("randompair", test2);

  std::vector<std::string> dict;
  for (size_t i = 0; i < test.size(); ++i) dict.push_back(std::to_string(i));

  std::vector<PairString> test3(test.size());
  for (size_t i = 0; i < test.size(); ++i)
    test3[i].key = dict[pcg32_random_r(&rng) % dict.size()].c_str();
  bench("randomstrp", test3);

  std::vector<float> test4(test.size());
  for (size_t i = 0; i < test.size(); ++i)
    test4[i] = float(pcg32_random_r(&rng) % test.size());
  bench("random flt", test4);

  std::vector<std::string> test5(test.size());
  for (size_t i = 0; i < test.size(); ++i)
    test5[i] = "longprefixtopushtoheap" + std::to_string(pcg32_random_r(&rng));
  bench("randomstr!", test5);
}
