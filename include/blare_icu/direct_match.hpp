#include <utility>
#include <vector>
#include <chrono>

#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>

#ifndef BLARE_RE2_DIRECT_MATCH_HPP_
#define BLARE_RE2_DIRECT_MATCH_HPP_

std::pair<double, int> DirectMatchICU (const std::vector<icu_72::UnicodeString> & lines, icu_72::UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;

    icu_72::RegexPattern* reg = icu_72::RegexPattern::compile(reg_string, 0, status);
    icu_72::RegexMatcher* matcher = reg->matcher(status);

    for (const auto & line : lines) {
        matcher->reset(line);
        count += matcher->find(status);
    }
    delete matcher;
    delete reg;
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_pair(elapsed_seconds.count(), count);
}

#endif // BLARE_RE2_DIRECT_MATCH_HPP_