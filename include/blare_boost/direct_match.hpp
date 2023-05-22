#include <utility>
#include <vector>
#include <string>
#include <chrono>
#include <boost/regex.hpp>

#ifndef BLARE_RE2_DIRECT_MATCH_HPP_
#define BLARE_RE2_DIRECT_MATCH_HPP_

std::pair<double, int> DirectMatchBoost (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    boost::smatch what;
    boost::regex reg{reg_string};
    for (const auto & line : lines) {
        count += boost::regex_search(line, what, reg);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_pair(elapsed_seconds.count(), count);
}

#endif // BLARE_RE2_DIRECT_MATCH_HPP_