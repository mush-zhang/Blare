#include <chrono>
#include <ctime> // seeding random number generators

#include <boost/random.hpp>
#include <boost/regex.hpp>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_boost/blare.hpp>

bool MultiMatchSingle (const std::string & line, std::vector<std::shared_ptr<boost::regex>> & c_regs, std::shared_ptr<boost::regex> & reg0, const std::vector<std::string> prefixes, const std::vector<std::string> & regs, bool prefix_first, std::vector<size_t> & prev_prefix_pos) {
    boost::smatch what;
    bool match = false;
    if (regs.empty()) {
        return line.find(prefixes[0]) != std::string::npos;
    } else if (prefixes.empty()) {
        return boost::regex_search(line, what, *reg0);
    } else {
        size_t pos = 0;
        size_t curr_prefix_pos = 0;
        size_t prefix_idx = 0;
        size_t reg_idx = 0;
        MATCH_LOOP_SINGLE:
            for (; prefix_idx < prefixes.size(); ) {
                // find pos of prefix before reg
                if ((curr_prefix_pos = line.find(prefixes[prefix_idx], pos)) == std::string::npos) {
                    if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                        goto CONTINUE_OUTER_SINGLE;
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
                    pos = prev_prefix_pos[reg_idx]+1;
                    prefix_idx = reg_idx;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            if (prefixes.size() == regs.size() && prefix_first) {
                auto start = line.begin() + pos;
                auto end = line.end();
                if (!boost::regex_search(start, end, what, *(c_regs.back()), boost::regex_constants::match_continuous)) {
                    prefix_idx = prev_prefix_pos.size() -1;
                    pos = prev_prefix_pos[prefix_idx]+1;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            if (!prefix_first) {
                auto curr = line.substr(0,prev_prefix_pos[0]);
                if (!boost::regex_search(curr, what, *reg0)){
                    prefix_idx = 0;
                    pos = prev_prefix_pos[prefix_idx]+1;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            match = true;                
        CONTINUE_OUTER_SINGLE:;
        std::fill(prev_prefix_pos.begin(), prev_prefix_pos.end(), 0);
    }
    return match;
}

bool SplitMatchSingle (const std::string & line, boost::regex & reg, const std::tuple<std::string, std::string, std::string> & r) {
    bool match = false;
    boost::smatch what;
    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    if (std::get<1>(r).empty()) {
        match = line.find(prefix) != std::string::npos;
    } else if (prefix.empty()) {
        if (suffix.empty()) {
            match = boost::regex_search(line, what, reg);
        } else {
            std::size_t pos = 0;
            while ((pos = line.find(suffix, pos)) != std::string::npos) {
                auto start = line.begin();
                auto end = line.begin() + pos;
                while (boost::regex_search(start, end, what, reg, boost::regex_constants::match_continuous)) {
                    if (what.suffix().str().empty()) {
                        match = true;
                        goto NEXT_LINE2;
                    } else {
                        start = what.suffix().first;
                    }
                }
                pos++;
            }
            NEXT_LINE2:;
        }
    } else if (suffix.empty()) {
        std::size_t pos = 0;
        while ((pos = line.find(prefix, pos)) != std::string::npos) {
            // for accuracy, should use line = line.substr(pos+1) next time
            auto start = line.begin() + pos + prefix.length();
            auto end = line.end();
            if (boost::regex_search(start, end, what, reg, boost::regex_constants::match_continuous)) {
                match = true;
                break;
            }
            pos++;
        }                       
    } else {
        std::size_t pos = 0;
        while ((pos = line.find(prefix, pos)) != std::string::npos) {
            std::size_t reg_start_pos = pos + prefix.length();
            std::size_t reg_end_pos = reg_start_pos;
            while ((reg_end_pos = line.find(suffix, reg_end_pos)) != std::string::npos) {
                if (boost::regex_match(line.substr(reg_start_pos, reg_end_pos - reg_start_pos ), what, reg)) {
                    match = true;
                    goto NEXT_LINE;
                }
                reg_end_pos++;
            }
            pos++;
        } 
        NEXT_LINE:;  
    }

    return match;
}

bool FullMatchSingle (const std::string & line, boost::regex & reg) {
    boost::smatch what;
    return boost::regex_search(line, what, reg);
}

std::tuple<double, int, unsigned int> BlareBoost (const std::vector<std::string> & lines, std::string reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    unsigned int idx = 0;

    // pre_compile regs
    boost::regex reg_full{reg_string};

    auto r = split_regex(reg_string);
    boost::regex reg_suffix{std::get<1>(r)};


    // Assumes prefix at first
    auto r_multi = split_regex_multi(reg_string);
    std::vector<std::string> prefixes = std::get<0>(r_multi);
    std::vector<std::string> regs_temp = std::get<1>(r_multi);
    auto regs = std::get<1>(r_multi);
    bool prefix_first = std::get<2>(r_multi);
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
    auto arm_num = 3;
    std::vector<int> pred_results{0, 0, 0};

    size_t skip_size = kSkipSize;
    size_t iteration_num = kEnembleNum;
    size_t sample_size = lines.size() / kSampleDivisor;


    if (sample_size < kSampleSizeLowerBound) {
        if (lines.size() >= 2*kSampleSizeLowerBound) {
            sample_size = kSampleSizeLowerBound;
            skip_size = 10;
        } else {
            // Use default multisplit
            pred_results[ARM::kMultiMatch] = 1;
            goto ARM_DECIDED;
        }
    } else if (sample_size > kSampleSizeUpperBound) {
        sample_size = kSampleSizeUpperBound;
    }

    boost::random::mt19937 gen(static_cast<std::uint32_t>(std::time(0)));
    boost::random::uniform_int_distribution<> dist{0, 2};

    std::vector<size_t> prev_prefix_pos(prefixes.size(), 0);

    for (; idx < skip_size; idx++) {
        switch(dist(gen)) {
            case ARM::kSplitMatch: count += SplitMatchSingle(lines[idx], reg_suffix, r); break;
            case ARM::kMultiMatch: count += MultiMatchSingle(lines[idx], c_regs, reg0, prefixes, regs, prefix_first, prev_prefix_pos); break;
            case ARM::kDirectMatch: count += FullMatchSingle(lines[idx], reg_full); break;
        }
    }

    for (size_t j = 0; j < iteration_num; j++) {
        auto trials = std::vector<unsigned int>(arm_num);
        auto ave_elapsed = std::vector<double>(arm_num);
        auto wins = std::vector<unsigned int>(arm_num);
        std::vector<boost::random::beta_distribution<>> prior_dists;
        for (size_t i = 0; i < arm_num; i++) {
            prior_dists.push_back(boost::random::beta_distribution<>(1, 1));
        }
        for (size_t k = 0; k < sample_size; k++, idx++) {
            std::vector<double> priors;
            for (auto& dist : prior_dists) {
                priors.push_back(dist(gen));
            }
            // Select the bandit that has the highest sampled value from the prior
            size_t chosen_bandit = argmax(priors);
            
            // Pull the lever of the chosen bandit
            auto single_start = std::chrono::high_resolution_clock::now();
            switch(chosen_bandit) {
                case ARM::kSplitMatch: count += SplitMatchSingle(lines[idx], reg_suffix, r); break;
                case ARM::kMultiMatch: count += MultiMatchSingle(lines[idx], c_regs, reg0, prefixes, regs, prefix_first, prev_prefix_pos); break;
                case ARM::kDirectMatch: count += FullMatchSingle(lines[idx], reg_full); break;
            }
            
            auto single_end = std::chrono::high_resolution_clock::now();
            auto single_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(single_end - single_start).count(); 
            trials[chosen_bandit] += 1;

            ave_elapsed[chosen_bandit] = (ave_elapsed[chosen_bandit] * (trials[chosen_bandit]-1) + single_elapsed) / ( 1.0 * trials[chosen_bandit]);
            wins[chosen_bandit] += argmin(ave_elapsed) == chosen_bandit;

            if (wins[chosen_bandit] >= sample_size / arm_num) 
                break;

            // Update the prior distribution of the chosen bandit
            auto alpha = 1 + wins[chosen_bandit];
            auto beta = 1 + trials[chosen_bandit] - wins[chosen_bandit];
            prior_dists[chosen_bandit] = boost::random::beta_distribution<>(alpha, beta);
        }
        auto winning_strategy = argmax(std::vector<double>{(wins[0]*1.0)/trials[0], (wins[1]*1.0)/trials[1], (wins[2]*1.0)/trials[2]});
        pred_results[winning_strategy]++;
        if (pred_results[winning_strategy] >= iteration_num / arm_num) 
            break;
    }

    ARM_DECIDED:;

    auto chosen_bandit = argmax(pred_results);
    boost::smatch what;
    switch(chosen_bandit) {
        case ARM::kSplitMatch: {
            auto prefix = std::get<0>(r);
            auto suffix = std::get<2>(r);
            if (std::get<1>(r).empty()) {
                for (; idx < lines.size(); idx++) {
                    count += lines[idx].find(prefix) != std::string::npos;
                }
            } else if (prefix.empty()) {
                if (suffix.empty()) {
                    for (; idx < lines.size(); idx++) {
                        count += boost::regex_search(lines[idx], what, reg_suffix);
                    }
                } else {
                    for (; idx < lines.size(); idx++) {
                        std::size_t pos = 0;
                        while ((pos = lines[idx].find(suffix, pos)) != std::string::npos) {
                            auto start = lines[idx].begin();
                            auto end = lines[idx].begin()+pos;
                            while (boost::regex_search(start, end, what, reg_suffix, boost::regex_constants::match_continuous)) {
                                if (what.suffix().str().empty()) {
                                    count++;
                                    goto NEXT_LINE2;
                                } else {
                                    start = what.suffix().first;
                                }
                            }
                            pos++;
                        }
                        NEXT_LINE2:;
                    }
                }
            } else if (suffix.empty()) {
                for (; idx < lines.size(); idx++) {
                    std::size_t pos = 0;
                    while ((pos = lines[idx].find(prefix, pos)) != std::string::npos) {
                        auto start = lines[idx].begin() + pos + prefix.length();
                        auto end = lines[idx].end();
                        if (boost::regex_search(start, end, what, reg_suffix, boost::regex_constants::match_continuous)) {
                            count++;
                            break;
                        }
                        pos++;
                    }
                }  
            } else {
                for (; idx < lines.size(); idx++) {
                    std::size_t pos = 0;
                    while ((pos = lines[idx].find(prefix, pos)) != std::string::npos) {
                        std::size_t reg_start_pos = pos + prefix.length();
                        std::size_t reg_end_pos = reg_start_pos;
                        while ((reg_end_pos = lines[idx].find(suffix, reg_end_pos)) != std::string::npos) {
                            if (boost::regex_match(lines[idx].substr(reg_start_pos, reg_end_pos - reg_start_pos), what, reg_suffix)) {
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
            break;
        }
        case ARM::kMultiMatch: {
            if (regs.empty()) {
                for (; idx < lines.size(); idx++) {
                    count += lines[idx].find(prefixes[0]) != std::string::npos;
                }
            } else if (prefixes.empty()) {
                boost::regex reg{regs[0]};
                for (; idx < lines.size(); idx++) {
                    count += boost::regex_search(lines[idx], what, reg);
                }
            } else {
                std::vector<size_t> prev_prefix_pos(prefixes.size(), 0);
                for (; idx < lines.size(); idx++) {
                    auto line = lines[idx];
                    size_t pos = 0;
                    size_t curr_prefix_pos = 0;
                    size_t prefix_idx = 0;
                    size_t reg_idx = 0;
                    MATCH_LOOP_BLARE:
                        for (; prefix_idx < prefixes.size(); ) {
                            // find pos of prefix before reg
                            if ((curr_prefix_pos = line.find(prefixes[prefix_idx], pos)) == std::string::npos) {
                                if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                                    goto CONTINUE_OUTER_BLARE;
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
                                pos = prev_prefix_pos[reg_idx]+1;
                                prefix_idx = reg_idx;
                                goto MATCH_LOOP_BLARE;
                            }
                        }
                        if (prefixes.size() == regs.size() && prefix_first) {
                            auto curr = line.substr(pos);
                            if (!boost::regex_search(curr, what, *(c_regs.back()), boost::regex_constants::match_continuous)) {
                                prefix_idx = prev_prefix_pos.size() -1;
                                pos = prev_prefix_pos[prefix_idx]+1;
                                goto MATCH_LOOP_BLARE;
                            } 
                        }
                        if (!prefix_first) {
                            auto curr = line.substr(0,prev_prefix_pos[0]);
                            if (!boost::regex_search(curr, what, *reg0)){
                                prefix_idx = 0;
                                pos = prev_prefix_pos[prefix_idx]+1;
                                goto MATCH_LOOP_BLARE;
                            }
                        }
                        count++;            
                    CONTINUE_OUTER_BLARE:;
                    std::fill(prev_prefix_pos.begin(), prev_prefix_pos.end(), 0);
                }
            }
            break;
        }
        case ARM::kDirectMatch: {
            for (; idx < lines.size(); idx++) {
                count += boost::regex_search(lines[idx], what, reg_full);
            } 
            break;
        }
    }

    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_tuple(elapsed_seconds.count(), count, chosen_bandit);
}