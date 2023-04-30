#include <algorithm>
#include <vector>

#ifndef BLARE_MISC_MISC_HPP_
#define BLARE_MISC_MISC_HPP_

template<class T>
size_t argmax(const std::vector<T>& v){
  return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

template<class T>
size_t argmin(const std::vector<T>& v){
  return std::distance(v.begin(), std::min_element(v.begin(), v.end()));
}

enum ARM {
  kSplitMatch = 0,
  kMultiMatch,
  kDirectMatch,
};

constexpr size_t kSkipSize = 100;
constexpr size_t kEnembleNum = 10;

constexpr size_t kSampleDivisor = 100000;
constexpr size_t kSampleSizeLowerBound = 200;
constexpr size_t kSampleSizeUpperBound = 10000;

#endif // BLARE_MISC_MISC_HPP_