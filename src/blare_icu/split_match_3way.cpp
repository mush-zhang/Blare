#include <chrono>

#include <boost/random.hpp>

#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>

#include <misc/misc.hpp>
#include <blare_icu/unicode_misc/split_regex.hpp>
#include <blare_icu/split_match_3way.hpp>

using namespace icu_72;

std::pair<double, int> SplitMatch3WayICU (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;

    auto r = split_regex(reg_string);

    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    RegexPattern* reg = RegexPattern::compile(std::get<1>(r), 0, status);
    RegexMatcher* matcher = reg->matcher(status);

    if (std::get<1>(r).isEmpty()) {
        for (const auto & line : lines) {
            count += line.indexOf(prefix) != -1;
        }
    } else if (prefix.isEmpty()) {
        if (suffix.isEmpty()) {
            for (const auto & line : lines) {
                matcher->reset(line);
                count += matcher->find(status);
            }
        } else {
            for (const auto & line : lines) {
                std::size_t pos = 0;
                while ((pos = line.indexOf(suffix, pos)) != -1) {
                    auto curr = line.tempSubString(0, pos);
                    matcher->reset(curr);
                    while (matcher->lookingAt(status)) {
                        if (matcher->end(status) == pos) {
                            count++;
                            goto NEXT_LINE2;
                        }
                    }
                    pos++;
                }
                NEXT_LINE2:;
            }
            
        }
    } else if (suffix.isEmpty()) {
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.indexOf(prefix, pos)) != -1) {
                matcher->reset(line);
                if (matcher->lookingAt(pos + prefix.length(), status)) {
                    count++;
                    break;
                }
                pos++;
            }                       
        }   
    } else {
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.indexOf(prefix, pos)) != -1) {
                std::size_t reg_start_pos = pos + prefix.length();
                std::size_t reg_end_pos = reg_start_pos;
                while ((reg_end_pos = line.indexOf(suffix, reg_end_pos)) != -1) {
                    auto curr = line.tempSubString(reg_start_pos, reg_end_pos - reg_start_pos);
                    matcher->reset(curr);
                    if (matcher->matches(status)) {
                        count++;
                        goto NEXT_LINE;
                    }
                    reg_end_pos++;
                }
                pos++;
            } 
            NEXT_LINE:;                      
        }
    }

    delete matcher;
    delete reg;
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_pair(elapsed_seconds.count(), count);
}