#include <tuple>
#include <vector>
#include <unicode/unistr.h>

#ifndef BLARE_ICU_UNICODE_MISC_REGEX_SPLIT_HPP_
#define BLARE_ICU_UNICODE_MISC_REGEX_SPLIT_HPP_

// 3way-split
std::tuple<icu_72::UnicodeString, icu_72::UnicodeString, icu_72::UnicodeString> split_unicode_regex(const icu_72::UnicodeString &line);

// multi-way-split
std::tuple<std::vector<icu_72::UnicodeString>, std::vector<icu_72::UnicodeString>, bool> split_unicode_regex_multi(const icu_72::UnicodeString &line);


#endif // BLARE_ICU_UNICODE_MISC_REGEX_SPLIT_HPP_