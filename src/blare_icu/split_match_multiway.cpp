#include <chrono>
#include <memory>

#include <boost/random.hpp>

#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>

#include <misc/misc.hpp>
#include <blare_icu/unicode_misc/split_regex.hpp>
#include <blare_re2/split_match_multiway.hpp>

using namespace icu_72;

std::pair<double, int> SplitMatchMultiWayICU (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;

    auto r = split_unicode_regex_multi(reg_string);

    std::vector<UnicodeString> prefixes = std::get<0>(r);
    std::vector<UnicodeString> regs = std::get<1>(r);
    std::vector<UnicodeString> regs_temp = std::get<1>(r);

    bool prefix_first = std::get<2>(r);

    std::shared_ptr<RegexPattern> reg0;
    std::shared_ptr<RegexMatcher> matcher0;

    if (!prefix_first) {
        reg0 = std::shared_ptr<RegexPattern>(RegexPattern::compile(regs_temp[0]+"$", 0, status));
        matcher0  = std::shared_ptr<RegexMatcher>(reg0->matcher(status));
        regs_temp.erase(regs_temp.begin());
    }
    std::vector<std::shared_ptr<RegexPattern>> c_regs;
    std::vector<std::shared_ptr<RegexMatcher>> matchers;
    for (const auto & reg : regs_temp) {
        std::shared_ptr<RegexPattern> c_reg = std::shared_ptr<RegexPattern>(RegexPattern::compile(reg, 0, status));
        c_regs.push_back(c_reg);
        std::shared_ptr<RegexMatcher> matcher = std::shared_ptr<RegexMatcher>(c_reg->matcher(status));
        matchers.push_back(matcher);
    }

    if (regs_temp.empty()) {
        for (const auto & line : lines) {
            count += line.indexOf(prefixes[0]) != -1;
        }
    } else if (prefixes.empty()) {
        RegexPattern* reg = RegexPattern::compile(regs_temp[0], 0, status);
        RegexMatcher* matcher = reg->matcher(status);

        for (const auto & line : lines) {
                matcher->reset(line);
                count += matcher->find(status);        
            }
        delete matcher;
        delete reg;
    } else {
        std::vector<size_t> prev_prefix_pos(prefixes.size(), 0);
        for (const auto & line : lines) {
            size_t pos = 0;
            size_t curr_prefix_pos = 0;
            size_t prefix_idx = 0;
            size_t reg_idx = 0;
            MATCH_LOOP:
                for (; prefix_idx < prefixes.size(); ) {
                    // find pos of prefix before reg
                    if ((curr_prefix_pos = line.indexOf(prefixes[prefix_idx], pos)) == -1) {
                        if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                            goto CONTINUE_OUTER;
                        else {
                            prefix_idx--;
                            reg_idx--;
                            pos = prev_prefix_pos[prefix_idx]+1;
                            continue;
                        }
                    }  
                    pos = curr_prefix_pos + prefixes[prefix_idx].length();
                    prev_prefix_pos[prefix_idx] = curr_prefix_pos;
                    if (prefix_idx == prev_prefix_pos.size()-1 || prev_prefix_pos[prefix_idx+1] >= pos) {
                        break;
                    }
                    prefix_idx++;
                }

                for (; reg_idx < prev_prefix_pos.size()-1; reg_idx++) {
                    size_t prev_prefix_end_pos = prev_prefix_pos[reg_idx] + prefixes[reg_idx].length();
                    auto curr = line.tempSubString(prev_prefix_end_pos, prev_prefix_pos[reg_idx+1] - prev_prefix_end_pos);
                    matchers[reg_idx]->reset(curr);
                    if (!matchers[reg_idx]->matches(status)){
                        pos = prev_prefix_pos[reg_idx+1]+1;
                        prefix_idx = reg_idx+1;
                        goto MATCH_LOOP;
                    }
                }
                if (prefixes.size() == regs.size() && prefix_first) {
                    auto curr = line.tempSubString(pos);
                    matchers.back()->reset(curr);
                    if (!matchers.back()->lookingAt(status)) {
                        prefix_idx = prev_prefix_pos.size() -1;
                        pos = prev_prefix_pos[prefix_idx]+1;
                        goto MATCH_LOOP;
                    } 
                }
                if (!prefix_first) {
                    auto curr = line.tempSubString(0,prev_prefix_pos[0]);
                    matcher0->reset(curr);
                    if (!matcher0->find(status)){
                        prefix_idx = 0;
                        pos = prev_prefix_pos[prefix_idx]+1;
                        goto MATCH_LOOP;
                    }
                }
                count++;      
            CONTINUE_OUTER:;
            std::fill(prev_prefix_pos.begin(), prev_prefix_pos.end(), 0);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_pair(elapsed_seconds.count(), count);
}