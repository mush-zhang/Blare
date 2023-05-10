#include <tuple>
#include <utility>
#include <vector>
#include <unicode/unistr.h>

#ifndef BLARE_ICU_BLARE_HPP_
#define BLARE_ICU_BLARE_HPP_

std::tuple<double, int, unsigned int> BlareICU (const std::vector<icu_72::UnicodeString> & lines, icu_72::UnicodeString reg_string) ;

#endif // BLARE_ICU_BLARE_HPP_