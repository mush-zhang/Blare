#include <tuple>
#include <vector>
#include <string>

#ifndef BLARE_PCRE2_BLARE_HPP_
#define BLARE_PCRE2_BLARE_HPP_

std::tuple<double, int, unsigned int> BlarePCRE2 (const std::vector<std::string> & lines, std::string reg_string);

#endif // BLARE_PCRE2_BLARE_HPP_