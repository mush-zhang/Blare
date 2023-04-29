#include <chrono>

#include <re2/re2.h>

#include "blare/misc.hpp"
#include "blare/split_regex.hpp"

#include "blare/re2_multi.hpp"

std::pair<double, int> MultiSplitMatchTest (const std::vector<std::string> & lines, std::string reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    auto r = split_regex_multi(reg_string);

    std::vector<std::string> prefixes = std::get<0>(r);
    std::vector<std::string> regs = std::get<1>(r);
    std::vector<std::string> regs_temp = std::get<1>(r);

    bool prefix_first = std::get<2>(r);
    std::shared_ptr<RE2> reg0;


    if (!prefix_first) {
        reg0 = std::shared_ptr<RE2>(new RE2(regs_temp[0]+"$"));
        regs_temp.erase(regs_temp.begin());
    }
    std::vector<std::shared_ptr<RE2>> c_regs;
    for (const auto & reg : regs_temp) {
        std::shared_ptr<RE2> c_reg(new RE2(reg));
        c_regs.push_back(c_reg);
    }

    std::string sm;

    if (regs_temp.empty()) {
        for (const auto & line : lines) {
            count += line.find(prefixes[0]) != std::string::npos;
        }
    } else if (prefixes.empty()) {
        RE2 reg(regs_temp[0]);
        for (const auto & line : lines) {
            count += RE2::PartialMatch(line, reg, &sm);
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
                        if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                            goto CONTINUE_OUTER;
                        else {
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
                    if (!RE2::FullMatch(curr, *(c_regs[reg_idx]), &sm)){
                        pos = prev_prefix_pos[reg_idx+1]+1;
                        prefix_idx = reg_idx;
                        goto MATCH_LOOP;
                    }
                }
                if (prefixes.size() == regs.size() && prefix_first) {
                    auto curr = line.substr(pos);
                    re2::StringPiece input(curr);
                    if (!RE2::Consume(&input, *(c_regs.back()), &sm)) {
                        prefix_idx = prev_prefix_pos.size() -1;
                        pos = prev_prefix_pos[prefix_idx]+1;
                        goto MATCH_LOOP;
                    } 
                }
                if (!prefix_first) {
                    auto curr = line.substr(0,prev_prefix_pos[0]);
                    if (!RE2::PartialMatch(curr, *reg0, &sm)){
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