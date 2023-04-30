#include <tuple>
#include <vector>
#include <string>

#ifndef MISC_REGEX_SPLIT_HPP_
#define MISC_REGEX_SPLIT_HPP_

// 3way-split
std::tuple<std::string, std::string, std::string> split_regex(const std::string &line);

// multi-way-split
std::vector<std::string> split_regex_multi(const std::string &line);


#endif // MISC_REGEX_SPLIT_HPP_