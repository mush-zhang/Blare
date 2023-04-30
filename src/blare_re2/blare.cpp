#include <chrono>
#include <ctime> // seeding random number generators

#include <re2/re2.h>
#include <boost/random.hpp>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_re2/blare.hpp>

bool MultiMatchSingle (const std::string & line, std::vector<std::shared_ptr<RE2>> & c_regs, std::shared_ptr<RE2> & reg0, const std::vector<std::string> prefixes, const std::vector<std::string> & regs, bool prefix_first, std::vector<size_t> & prev_prefix_pos) {
    std::string sm;
    bool match = false;
    if (regs.empty()) {
        return line.find(prefixes[0]) != std::string::npos;
    } else if (prefixes.empty()) {
        return RE2::PartialMatch(line, *reg0, &sm);
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
                if (!RE2::FullMatch(curr, *(c_regs[reg_idx]), &sm)){
                    pos = prev_prefix_pos[reg_idx]+1;
                    prefix_idx = reg_idx;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            if (prefixes.size() == regs.size() && prefix_first) {
                auto curr = line.substr(pos);
                re2::StringPiece input(curr);
                if (!RE2::Consume(&input, *(c_regs.back()), &sm)) {
                    prefix_idx = prev_prefix_pos.size() -1;
                    pos = prev_prefix_pos[prefix_idx]+1;
                    goto MATCH_LOOP_SINGLE;
                } 
            }
            if (!prefix_first) {
                auto curr = line.substr(0,prev_prefix_pos[0]);
                if (!RE2::PartialMatch(curr, *reg0, &sm)){
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

bool SplitMatchSingle (const std::string & line, RE2 & reg, const std::tuple<std::string, std::string, std::string> & r) {
    bool match = false;
    std::string sm;
    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    if (std::get<1>(r).empty()) {
        match = line.find(prefix) != std::string::npos;
    } else if (prefix.empty()) {
        if (suffix.empty()) {
            match = RE2::PartialMatch(line, reg, &sm);
        } else {
            std::size_t pos = 0;
            while ((pos = line.find(suffix, pos)) != std::string::npos) {
                std::string curr_in = line.substr(0, pos); 
                re2::StringPiece input(curr_in);
                while (RE2::Consume(&input, reg, &sm)) {
                    if (input.ToString().empty()) {
                        match = true;
                        goto NEXT_LINE2;
                    } 
                }
                pos++;
            }
            NEXT_LINE2:;
        }
    } else if (suffix.empty()) {
        std::size_t pos = 0;
        while ((pos = line.find(prefix, pos)) != std::string::npos) {
            std::string curr_in = line.substr(pos + prefix.length()); 
            re2::StringPiece input(curr_in);
            if (RE2::Consume(&input, reg, &sm)) {
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
                if (RE2::FullMatch(line.substr(reg_start_pos, reg_end_pos - reg_start_pos ), reg, &sm)) {
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

bool FullMatchSingle (const std::string & line, RE2 & reg) {
    std::string sm;
    return RE2::PartialMatch(line, reg, &sm);
}

std::tuple<double, int, unsigned int> BlareRe2 (const std::vector<std::string> & lines, std::string reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    unsigned int idx = 0;

    // pre_compile regs
    RE2 reg_full(reg_string);

    auto r = split_regex(reg_string);
    RE2 reg_suffix(std::get<1>(r));

    // Assumes prefix at first
    auto r_multi = split_regex_multi(reg_string);
    std::vector<std::string> prefixes = std::get<0>(r_multi);
    std::vector<std::string> regs_temp = std::get<1>(r_multi);
    auto regs = std::get<1>(r_multi);
    bool prefix_first = std::get<2>(r_multi);
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
        // Number of trials per bandit
        auto trials = std::vector<unsigned int>(arm_num);
        // Average time per bandit
        auto ave_elapsed = std::vector<double>(arm_num);
        // Number of wins per bandit
        auto wins = std::vector<unsigned int>(arm_num);
        // Beta distributions of the priors for each bandit
        std::vector<boost::random::beta_distribution<>> prior_dists;
        // Initialize the prior distributions with alpha=1 beta=1
        for (size_t i = 0; i < arm_num; i++) {
            prior_dists.push_back(boost::random::beta_distribution<>(1, 1));
        }
        for (size_t k = 0; k < sample_size; k++, idx++) {
            std::vector<double> priors;
            // Sample a random value from each prior distribution.
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
    std::string sm;
    switch(chosen_bandit) {
        case kSplitMatch: {
            auto prefix = std::get<0>(r);
            auto suffix = std::get<2>(r);
            if (std::get<1>(r).empty()) {
                for (; idx < lines.size(); idx++) {
                    count += lines[idx].find(prefix) != std::string::npos;
                }
            } else if (prefix.empty()) {
                if (suffix.empty()) {
                    for (; idx < lines.size(); idx++) {
                        count += RE2::PartialMatch(lines[idx], reg_suffix, &sm);
                    }
                } else {
                    for (; idx < lines.size(); idx++) {
                        std::size_t pos = 0;
                        while ((pos = lines[idx].find(suffix, pos)) != std::string::npos) {
                            std::string curr_in = lines[idx].substr(0, pos); 
                            re2::StringPiece input(curr_in);
                            while (RE2::Consume(&input, reg_suffix, &sm)) {
                                if (input.ToString().empty()) {
                                    count++;
                                    goto NEXT_LINE2;
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
                        std::string curr_in = lines[idx].substr(pos + prefix.length()); 
                        re2::StringPiece input(curr_in);
                        if (RE2::Consume(&input, reg_suffix, &sm)) {
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
                            if (RE2::FullMatch(lines[idx].substr(reg_start_pos, reg_end_pos - reg_start_pos ), reg_suffix, &sm)) {
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
        case kMultiMatch: {
            if (regs.empty()) {
                for (; idx < lines.size(); idx++) {
                    count += lines[idx].find(prefixes[0]) != std::string::npos;
                }
            } else if (prefixes.empty()) {
                RE2 reg(regs[0]);
                for (; idx < lines.size(); idx++) {
                    count += RE2::PartialMatch(lines[idx], reg, &sm);
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
                            if (!RE2::FullMatch(curr, *(c_regs[reg_idx]), &sm)){
                                pos = prev_prefix_pos[reg_idx]+1;
                                prefix_idx = reg_idx;
                                goto MATCH_LOOP_BLARE;
                            }
                        }
                        if (prefixes.size() == regs.size() && prefix_first) {
                            auto curr = line.substr(pos);
                            re2::StringPiece input(curr);
                            if (!RE2::Consume(&input, *(c_regs.back()), &sm)) {
                                prefix_idx = prev_prefix_pos.size() -1;
                                pos = prev_prefix_pos[prefix_idx]+1;
                                goto MATCH_LOOP_BLARE;
                            } 
                        }
                        if (!prefix_first) {
                            auto curr = line.substr(0,prev_prefix_pos[0]);
                            if (!RE2::PartialMatch(curr, *reg0, &sm)){
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
        case kDirectMatch: {
            for (; idx < lines.size(); idx++) {
                count += RE2::PartialMatch(lines[idx], reg_full, &sm);
            } 
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return std::make_tuple(elapsed_seconds.count(), count, chosen_bandit);
}