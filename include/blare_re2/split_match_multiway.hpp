#include <utility>
#include <vector>
#include <string>

#ifndef BLARE_RE2_SPLIT_MATCH_MULTIWAY_HPP_
#define BLARE_RE2_SPLIT_MATCH_MULTIWAY_HPP_

std::pair<double, int> SplitMatchMultiWayRE2 (const std::vector<std::string> & lines, const std::string & reg_string);

#endif // BLARE_RE2_SPLIT_MATCH_MULTIWAY_HPP_