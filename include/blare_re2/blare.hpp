#include <tuple>
#include <vector>
#include <string>

#ifndef BLARE_RE2_BLARE_HPP_
#define BLARE_RE2_BLARE_HPP_

std::tuple<double, int, unsigned int> BlareRe2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> BlareCountAllRe2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> BlareLongestRe2 (const std::vector<std::string> & lines, std::string reg_string);

std::tuple<double, int, unsigned int> Blare4ArmsRe2 (const std::vector<std::string> & lines, std::string reg_string);

#endif // BLARE_RE2_BLARE_HPP_