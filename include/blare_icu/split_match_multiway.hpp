#include <pair>
#include <vector>
#include <unicode/unistr.h>

#ifndef BLARE_ICU_SPLIT_MATCH_MULTIWAY_HPP_
#define BLARE_ICU_SPLIT_MATCH_MULTIWAY_HPP_

std::pair<double, int> SplitMatchMultiWayICU (const std::vector<icu_72::UnicodeString> & lines, icu_72::UnicodeString reg_string) ;

#endif // BLARE_ICU_SPLIT_MATCH_MULTIWAY_HPP_