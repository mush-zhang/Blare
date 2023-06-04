#include <chrono>
#include <memory>

#include <boost/random.hpp>
#include <boost/regex.hpp>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_boost/split_match_multiway.hpp>

std::pair<double, int> SplitMatchMultiWayBoost (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    auto r = split_regex_multi(reg_string);

    std::vector<std::string> prefixes = std::get<0>(r);
    std::vector<std::string> regs = std::get<1>(r);
    std::vector<std::string> regs_temp = std::get<1>(r);

    bool prefix_first = std::get<2>(r);
    std::shared_ptr<boost::regex> reg0;


    if (!prefix_first) {
        reg0 = std::shared_ptr<boost::regex>(new boost::regex{regs_temp[0]+"$"});
        regs_temp.erase(regs_temp.begin());
    }
    std::vector<std::shared_ptr<boost::regex>> c_regs;
    for (const auto & reg : regs_temp) {
        std::shared_ptr<boost::regex> c_reg(new boost::regex{reg});
        c_regs.push_back(c_reg);
    }

    boost::smatch what;

    if (regs_temp.empty()) {
        for (const auto & line : lines) {
            count += line.find(prefixes[0]) != std::string::npos;
        }
    } else if (prefixes.empty()) {
        boost::regex reg{regs_temp[0]};
        for (const auto & line : lines) {
            count += boost::regex_search(line, what, reg);
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
                    if (!boost::regex_match(curr, what, *(c_regs[reg_idx]))){
                        pos = prev_prefix_pos[reg_idx+1]+1;
                        prefix_idx = reg_idx+1;
                        goto MATCH_LOOP;
                    }
                }
                if (prefixes.size() == regs.size() && prefix_first) {
                    auto start = line.begin() + pos;
                    auto end = line.end();
                    if (!boost::regex_search(start, end, what, *(c_regs.back()), boost::regex_constants::match_continuous)) {
                        prefix_idx = prev_prefix_pos.size() -1;
                        pos = prev_prefix_pos[prefix_idx]+1;
                        goto MATCH_LOOP;
                    } 
                }
                if (!prefix_first) {
                    auto curr = line.substr(0,prev_prefix_pos[0]);
                    if (!boost::regex_search(curr, what, *reg0)){
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