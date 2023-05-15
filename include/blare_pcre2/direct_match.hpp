#include <utility>
#include <vector>
#include <string>
#include <chrono>

#include "jpcre2.hpp"

#ifndef BLARE_PCRE2_DIRECT_MATCH_HPP_
#define BLARE_PCRE2_DIRECT_MATCH_HPP_

std::pair<double, int> DirectMatchPCRE2 (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    jp::VecNum vec_num;
    jp::Regex re(reg_string, jpcre2::JIT_COMPILE);
    jp::RegexMatch rm(&re);

    for (const auto & line : lines) {
        count += rm.setSubject(line).setNumberedSubstringVector(&vec_num).match();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_pair(elapsed_seconds.count(), count);
}

#endif // BLARE_PCRE2_DIRECT_MATCH_HPP_