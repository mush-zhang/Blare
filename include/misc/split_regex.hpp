#include <tuple>
#include <utility>
#include <vector>
#include <string>

#ifndef MISC_SPLIT_REGEX_HPP_
#define MISC_SPLIT_REGEX_HPP_

// 3way-split
std::tuple<std::string, std::string, std::string> split_regex(const std::string &line);

// multi-way-split
std::tuple<std::vector<std::string>, std::vector<std::string>, bool> split_regex_multi(const std::string &line);


#endif // MISC_SPLIT_REGEX_HPP_