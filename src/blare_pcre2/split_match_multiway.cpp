#include <chrono>
#include <memory>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_pcre2/split_match_multiway.hpp>


#include "jpcre2.hpp"

typedef jpcre2::select<char> jp;

std::pair<double, int> SplitMatchMultiWayPCRE2 (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    jp::VecNum vec_num;

    auto r_multi = split_regex_multi(reg_string);
    std::vector<std::string> prefixes = std::get<0>(r_multi);
    std::vector<std::string> regs = std::get<1>(r_multi);
    std::vector<std::string> regs_temp = std::get<1>(r_multi);
    bool prefix_first = std::get<2>(r_multi);
    jp::Regex re0(regs_temp[0], PCRE2_ENDANCHORED, jpcre2::JIT_COMPILE);
    jp::RegexMatch reg0 = jp::RegexMatch(&re0);
    if (!prefix_first) {
        regs_temp.erase(regs_temp.begin());
    }
    std::vector<jp::Regex> c_regs;
    for (auto i = 0; i < regs_temp.size(); i++) {
        if (i == regs_temp.size() - 1 && regs_temp.size() == prefixes.size()) {
            jp::Regex re(regs_temp[i], PCRE2_ANCHORED, jpcre2::JIT_COMPILE);
            c_regs.push_back(re);
        } else {
            jp::Regex re(regs_temp[i], PCRE2_ANCHORED | PCRE2_ENDANCHORED, jpcre2::JIT_COMPILE);
            c_regs.push_back(re);
        }
    }

    if (regs.empty()) {
        for (const auto & line : lines) {
            count += line.find(prefixes[0]) != std::string::npos;
        }
    } else if (prefixes.empty()) {
        for (const auto & line : lines) {
            count += reg0.setSubject(line).setNumberedSubstringVector(&vec_num).match();
        }
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
                    if ((curr_prefix_pos = line.find(prefixes[prefix_idx], pos)) == std::string::npos) {
                        if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0) {
                            goto CONTINUE_OUTER;
                        } else {
                            prefix_idx--;
                            reg_idx--;
                            pos = prev_prefix_pos[prefix_idx]+1;
                            continue;
                        }
                    }
                    pos = curr_prefix_pos + prefixes[prefix_idx].size();
                    prev_prefix_pos[prefix_idx] = curr_prefix_pos;
                    if (prefix_idx == prev_prefix_pos.size()-1 || prev_prefix_pos[prefix_idx+1] >= pos) {
                        break;
                    }
                    prefix_idx++;
                }
                for (; reg_idx < prev_prefix_pos.size()-1; reg_idx++) {
                    size_t prev_prefix_end_pos = prev_prefix_pos[reg_idx] + prefixes[reg_idx].size();
                    auto curr = line.substr(prev_prefix_end_pos, prev_prefix_pos[reg_idx+1] - prev_prefix_end_pos);
                    jp::RegexMatch rm_temp(&c_regs[reg_idx]);
                    if (!rm_temp.setSubject(curr).setNumberedSubstringVector(&vec_num).match()){
                        pos = prev_prefix_pos[reg_idx]+1;
                        prefix_idx = reg_idx+1;
                        goto MATCH_LOOP;
                    }
                }
                if (prefixes.size() == regs.size() && prefix_first) {
                    auto curr = line.substr(pos);
                    jp::RegexMatch rm_temp(&c_regs.back());
                    if (!rm_temp.setSubject(curr).setNumberedSubstringVector(&vec_num).match()) {
                        prefix_idx = prev_prefix_pos.size() -1;
                        pos = prev_prefix_pos[prefix_idx]+1;
                        goto MATCH_LOOP;
                    } 
                }
                if (!prefix_first) {
                    auto curr = line.substr(0,prev_prefix_pos[0]);
                    if (!reg0.setSubject(curr).setNumberedSubstringVector(&vec_num).match()){
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