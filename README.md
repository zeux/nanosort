# nanosort [![Actions Status](https://github.com/zeux/nanosort/workflows/build/badge.svg)](https://github.com/zeux/nanosort/actions) ![MIT](https://img.shields.io/badge/license-MIT-blue.svg)

## Algorithm

nanosort aims to be a fast comparison-based sorting algorithm, tuned for POD types of reasonably small sizes. nanosort implements an algorithm similar to introsort - divide & conquer quick sort with small subarrays sorted using a quadratic sort, and a fallback to heap sort to guarantee worst case NlogN execution time. To get high performance, nanosort uses the following techniques:

- Instead of classical partition algorithms, nanosort uses a Lomuto-inspired branchless partition. Due to unique construction, this partition results in constant superb performance given minimal code size.
- Instead of classical insertion sort, nanosort uses a 2-at-a-time bubble sort discovered by Gerben Stavenga; branchless implementation of this algorithm similarly results in excellent performance with reasonable code size.

To reach high performance, it's critical that key loops in nanosort (`partition`, `small_sort`) as well as `median5` selection network are compiled using efficient branchless code, using instructions similar to `setb` and `cmov`. Not all compilers can do this properly; as such, nanosort presently has variable performance across different compilers.

Crucially, nanosort guarantees worst case complexity of NlogN and does not result in undefined behavior even when the comparison function doesn't use strict weak ordering. This is in stark contrast to most STL implementations (for example, `libc++` has a worst case complexity of O(N^2) on certain inputs; all STL implementations can crash, including out of bounds *writes*, when given an input array of floats that contains NaN values).

nanosort values predictability of execution time - most sequences of a given size and type are going to take more or less the same amount of time to sort. Because of this, nanosort can lose to algorithms that can detect sorted or partially sorted inputs, although even a fairly small number of random swaps in a sorted input are enough to make nanosort competitive with algorithms like pdqsort.

## Implementation

nanosort is implemented as a header-only library that should compile on any compiler that supports C++03; nanosort optionally supports C++11 (and will use move construction/assignment to reduce copy cost). nanosort has no dependencies, including STL.

nanosort compiles to ~1KB of x64 assembly code when using clang with -O2 and sorting an array of integers.

To use nanosort, include the header and call `nanosort` function with or without a comparator:

```c++
#include "nanosort.hpp"

...
nanosort(data, data + count);
nanosort(data, data + count, std::greater<int>());
...
```

## Benchmarks

All benchmarks were ran on Intel Core i7-8700K.

All benchmarks sort POD data types except for `randomstr!` which sorts std::string objects.

### clang 11 / libc++

nanosort performs very well on clang, beating other sorts most of the time with two notable exceptions:

- on "sorted int" and "sortre int" libc++ std::sort and pdqsort are linear and all other sorts here are NlogN
- on "randomstr!", the cost of extra std::string copies in nanosort outweighs all improvements

benchmark  | std::sort  | pdqsort    | exp_gerbens | nanosort
-----------|------------|------------|-------------|----------
random int | 2.61 ns/op | 1.26 ns/op | 0.86 ns/op | 0.79 ns/op
sorted int | 0.03 ns/op | 0.04 ns/op | 0.76 ns/op | 0.81 ns/op
sroted int | 0.46 ns/op | 0.79 ns/op | 11.03 ns/op | 0.69 ns/op
run100 int | 0.76 ns/op | 1.18 ns/op | 0.84 ns/op | 0.78 ns/op
sortre int | 0.07 ns/op | 0.08 ns/op | 6.98 ns/op | 0.74 ns/op
eq1000 int | 1.40 ns/op | 0.48 ns/op | 0.49 ns/op | 0.42 ns/op
randompair | 2.65 ns/op | 2.72 ns/op | 1.07 ns/op | 0.95 ns/op
randomstrp | 9.99 ns/op | 9.58 ns/op | 11.50 ns/op | 10.94 ns/op
random flt | 2.92 ns/op | 1.40 ns/op | 1.29 ns/op | 1.41 ns/op
randomstr! | 13.80 ns/op | 13.13 ns/op | 35.02 ns/op | 24.03 ns/op

### gcc 10 / libstdc++

gcc currently doesn't generate a proper branchless sequence for some of the algorithms, leading to worse performance on random benchmarks compared to clang. nanosort still has good performance but doesn't win as convincingly.

The author plans to look into tuning nanosort to generate better code on gcc.

benchmark  | std::sort  | pdqsort    | exp_gerbens | nanosort
-----------|------------|------------|-------------|----------
random int | 2.60 ns/op | 1.28 ns/op | 1.66 ns/op | 1.27 ns/op
sorted int | 0.54 ns/op | 0.03 ns/op | 1.06 ns/op | 0.85 ns/op
sroted int | 0.86 ns/op | 0.84 ns/op | 20.16 ns/op | 0.76 ns/op
run100 int | 0.98 ns/op | 1.22 ns/op | 1.30 ns/op | 0.96 ns/op
sortre int | 0.44 ns/op | 0.08 ns/op | 13.51 ns/op | 0.90 ns/op
eq1000 int | 1.60 ns/op | 0.48 ns/op | 0.86 ns/op | 0.46 ns/op
randompair | 2.77 ns/op | 2.77 ns/op | 1.79 ns/op | 2.48 ns/op
randomstrp | 9.59 ns/op | 8.90 ns/op | 12.09 ns/op | 11.11 ns/op
random flt | 2.89 ns/op | 1.33 ns/op | 1.83 ns/op | 1.44 ns/op
randomstr! | 15.29 ns/op | 12.82 ns/op | 35.67 ns/op | 22.12 ns/op

### MSVC 2019 / MSVC STL

MSVC does generate branchless code for most algorithms used by nanosort, but has several severe codegen performance issues that lead to excessive serialization of execution, which leads to much worse IPC on this code.

The author plans to get Microsoft to fix the code generation here, as it can be indicative of similar problems in other tight loops, and/or implement workarounds in nanosort code.

benchmark  | std::sort  | pdqsort    | exp_gerbens | nanosort
-----------|------------|------------|-------------|----------
random int | 3.18 ns/op | 2.63 ns/op | 1.97 ns/op | 2.00 ns/op
sorted int | 0.43 ns/op | 0.04 ns/op | 0.80 ns/op | 2.02 ns/op
sroted int | 0.84 ns/op | 0.59 ns/op | 9.65 ns/op | 1.77 ns/op
run100 int | 1.32 ns/op | 0.83 ns/op | 0.93 ns/op | 2.00 ns/op
sortre int | 0.51 ns/op | 0.05 ns/op | 9.16 ns/op | 1.92 ns/op
eq1000 int | 1.56 ns/op | 1.34 ns/op | 1.49 ns/op | 1.22 ns/op
randompair | 3.29 ns/op | 2.75 ns/op | 0.91 ns/op | 2.42 ns/op
randomstrp | 10.21 ns/op | 8.40 ns/op | 12.90 ns/op | 12.60 ns/op
random flt | 3.50 ns/op | 2.97 ns/op | 2.93 ns/op | 1.40 ns/op
randomstr! | 19.45 ns/op | 18.36 ns/op | 87.24 ns/op | 28.72 ns/op

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
