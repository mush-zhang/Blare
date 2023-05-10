#include <utility>
#include <vector>
#include <string>

#ifndef BLARE_RE2_BLARE_HPP_
#define BLARE_RE2_BLARE_HPP_

std::tuple<double, int, unsigned int> BlareRE2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> BlareCountAllRE2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> BlareLongestRE2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> Blare4ArmsRE2 (const std::vector<std::string> & lines, std::string reg_string);

#endif // BLARE_RE2_BLARE_HPP_