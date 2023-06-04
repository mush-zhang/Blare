#include <iostream>
#include <fstream>
// #include <filesystem>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <algorithm>
#include <regex>
#include <chrono>
#include <numeric>
#include <random>
#include <tuple>
#include <queue>
#include <cmath>
#include <limits>
#include <ctime>

#include <boost/regex.hpp>
#include <boost/random.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

// Define base_generator as a Mersenne Twister. This is needed only to make the
// code a bit less verbose.
typedef boost::mt19937 base_generator;

bool char_escaped(const std::string &line, std::size_t pos) {
    auto temp_pos = pos;
    int num_escaped = 0;
    // count number of escape characters before the char
    while (temp_pos > 0) {
        if (line.at(--temp_pos) != '\\') 
            break;
        num_escaped++;
    }
    return num_escaped % 2 == 1;
}


std::tuple<std::string, std::string, std::string> split_regex(const std::string &line) {
    std::size_t pos = 0;
    std::string prefix = line;
    std::string regex = "";
    std::string suffix = "";
    while ((pos = line.find("(", pos)) != std::string::npos) {
        if (!char_escaped(line, pos)) {
            std::size_t reg_start_pos = pos;
            std::size_t reg_end_pos = pos+1;
            while ((pos = line.find(")", pos+1)) != std::string::npos) {
                if (!char_escaped(line, pos)) {
                    reg_end_pos = pos;
                }
                pos++;
            }
            prefix = line.substr(0, reg_start_pos);
            regex = line.substr(reg_start_pos, reg_end_pos+1-reg_start_pos);
            suffix = line.substr(reg_end_pos+1);
            
            break;
        }
        pos++;
    }
    prefix.erase(std::remove(prefix.begin(), prefix.end(), '\\'), prefix.end()); // remove escape (trick method. should only remove single \ and turning \\ to \)
    suffix.erase(std::remove(suffix.begin(), suffix.end(), '\\'), suffix.end()); // remove escape (trick method. should only remove single \ and turning \\ to \)
    return std::make_tuple(prefix, regex, suffix);
}

std::tuple<std::vector<std::string>, std::vector<std::string>, bool> split_regex_multi(const std::string &line) {
    std::size_t pos = 0;
    std::size_t prev_pos = 0;
    int pos2 = -1;
    std::vector<std::string> const_strings;
    std::vector<std::string> regexes;
    std::string prefix = line;
    std::string suffix = "";
    bool prefix_first = true;
    while ((pos = line.find("(", pos)) != std::string::npos) {
        if (!char_escaped(line, pos)) {
            prefix = line.substr(prev_pos, pos-prev_pos);
            if (prev_pos == 0 && pos == 0) prefix_first = false;

            pos2 = pos;
            while ((pos2 = line.find(")", pos2)) != std::string::npos) {
                if (!char_escaped(line, pos2)) {
                    suffix = line.substr(pos, pos2 + 1 -pos);
                    break;
                }
                pos2++;
            }
            
            if (pos2 == std::string::npos && suffix.empty()) {
                suffix = line.substr(pos);
            }
            pos2++; 
            regexes.push_back(suffix);
            if (!prefix.empty()) {
                prefix.erase(std::remove(prefix.begin(), prefix.end(), '\\'), prefix.end()); // remove escape (trick method. should only remove single \ and turning \\ to \)
                const_strings.push_back(prefix);
            }
            
            prev_pos = pos2;
            pos = pos2;
        } else {
            pos++;

        }
    }
    prefix = line.substr(pos2);
    if (!prefix.empty()) {
        prefix.erase(std::remove(prefix.begin(), prefix.end(), '\\'), prefix.end()); // remove escape (trick method. should only remove single \ and turning \\ to \)
        const_strings.push_back(prefix);
    }
    return std::tuple<std::vector<std::string>, std::vector<std::string>, bool>(const_strings, regexes, prefix_first);
}

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
                    pos = prev_prefix_pos[reg_idx+1]+1;
                    prefix_idx = reg_idx+1;
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

// argmax returns the index of maximum element in vector v.
template<class T>
size_t argmax(const std::vector<T>& v){
  return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

// argmin returns the index of minimum element in vector v.
template<class T>
size_t argmin(const std::vector<T>& v){
  return std::distance(v.begin(), std::min_element(v.begin(), v.end()));
}

std::tuple<double, int, unsigned int> Blare (const std::vector<std::string> & lines, const std::string & reg_string) {
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
    std::vector<size_t> prev_prefix_pos(prefixes.size(), 0);

    size_t skip_size = 100;
    size_t iteration_num = 10;
    size_t sample_size = lines.size() / 100000;
    if (sample_size < 200) {
        sample_size = 200;
        skip_size = 10;
    }
    else if (sample_size > 10000) 
        sample_size = 10000;

    base_generator gen(static_cast<std::uint32_t>(std::time(0)));
    boost::random::uniform_int_distribution<> dist{0, 2};

    // auto pred_results = std::vector<int>(iteration_num);
    std::vector<int> pred_results{0, 0, 0};
    auto arm_num = 3;

    for (; idx < skip_size; idx++) {
        // switch(b(rng)) {
        switch(dist(gen)) {
            case 0: count += SplitMatchSingle(lines[idx], reg_suffix, r); break;
            case 1: count += MultiMatchSingle(lines[idx], c_regs, reg0, prefixes, regs, prefix_first, prev_prefix_pos); break;
            case 2: count += FullMatchSingle(lines[idx], reg_full); break;
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
                case 0: count += SplitMatchSingle(lines[idx], reg_suffix, r); break;
                case 1: count += MultiMatchSingle(lines[idx], c_regs, reg0, prefixes, regs, prefix_first, prev_prefix_pos); break;
                case 2: count += FullMatchSingle(lines[idx], reg_full); break;
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
        // pred_results[j] = winning_strategy;
        pred_results[winning_strategy]++;
        if (pred_results[winning_strategy] >= iteration_num / arm_num) 
            break;
    }
    auto chosen_bandit = argmax(pred_results);
    boost::smatch what;
    switch(chosen_bandit) {
        case 0: {
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
        case 1: {
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
                                pos = prev_prefix_pos[reg_idx+1]+1;
                                prefix_idx = reg_idx+1;
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
        case 2: {
            for (; idx < lines.size(); idx++) {
                count += boost::regex_search(lines[idx], what, reg_full);
            } 
            break;
        }
    }

    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << "Blare: " << count << std::endl;
    std::cout << "sample size: " << sample_size <<" chooses: " << chosen_bandit  << std::endl;
    return std::make_tuple(elapsed_seconds.count(), count, chosen_bandit);
}

std::pair<double, int> BoostFullAll (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    boost::smatch what;
    boost::regex reg{reg_string};
    for (const auto & line : lines) {
        count += boost::regex_search(line, what, reg);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << "direct: " << reg_string << " " << count << std::endl;
    return std::make_pair(elapsed_seconds.count(), count);
}

std::pair<double, int> SplitMatchAll (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    auto r = split_regex(reg_string);

    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    boost::regex reg{std::get<1>(r)};
    boost::smatch what;

    if (std::get<1>(r).empty()) {
        for (const auto & line : lines) {
            count += line.find(prefix) != std::string::npos;
        }
    } else if (prefix.empty()) {
        if (suffix.empty()) {
            for (const auto & line : lines) {
                count += boost::regex_search(line, what, reg);
            }
        } else {
            for (const auto & line : lines) {
                std::size_t pos = 0;
                while ((pos = line.find(suffix, pos)) != std::string::npos) {
                    auto start = line.begin();
                    auto end = line.begin() + pos;
                    while (boost::regex_search(start, end, what, reg, boost::regex_constants::match_continuous)) {
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
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.find(prefix, pos)) != std::string::npos) {
                auto start = line.begin() + pos + prefix.length();
                auto end = line.end();
                if (boost::regex_search(start, end, what, reg, boost::regex_constants::match_continuous)) {
                    count++;
                    break;
                }
                pos++;
            }                       
        }   
    } else {
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.find(prefix, pos)) != std::string::npos) {
                std::size_t reg_start_pos = pos + prefix.length();
                std::size_t reg_end_pos = reg_start_pos;
                while ((reg_end_pos = line.find(suffix, reg_end_pos)) != std::string::npos) {
                    if (boost::regex_match(line.substr(reg_start_pos, reg_end_pos - reg_start_pos ), what, reg)) {
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

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    std::cout << "split: " << reg_string << " " << count  << std::endl;

    return std::make_pair(elapsed_seconds.count(), count);
}

std::pair<double, int> MultiSplitMatchTest (const std::vector<std::string> & lines, const std::string & reg_string) {
    std::cout << reg_string << std::endl;
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
    
    for (const auto & p : prefixes) {
        std::cout << "[" << p << "]\t";  
    }
    std::cout << std::endl;

    std::cout << "Multi Split " << count << std::endl;

    return std::make_pair(elapsed_seconds.count(), count);
}

std::vector<std::string> read_sys_y() {
    std::string line;
    std::vector<std::string> lines;
    std::string data_file = "tagged_data.csv";
    std::cout << "reading: " << data_file <<  std::endl;

    std::ifstream data_in(data_file);
    if (!data_in.is_open()) {
        std::cerr << "Could not open the file - '" << data_file << "'" << std::endl;
        return lines;
    }
    int i = 0;
    while (getline(data_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        std::stringstream streamData(line);
        std::vector<std::string> curr_line;
        std::string s; 
        while (getline(streamData, s, '\t')) {
            curr_line.push_back(s);
        }
        curr_line.resize(curr_line.size()-4);

        std::string curr;
        for (const auto &piece : curr_line) curr += piece;

        lines.push_back(curr);
    }
    data_in.close();
    return lines;
}

std::vector<std::string> read_traffic() {
    std::string line;
    std::vector<std::string> lines;
    std::string data_file = "../BLARE_DATA/US_Accidents_Dec21_updated.csv";
    std::ifstream data_in(data_file);
    if (!data_in.is_open()) {
        std::cerr << "Could not open the file - '" << data_file << "'" << std::endl;
        return lines;
    }
    while (getline(data_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        std::stringstream streamData(line);
        std::string s; 
        int i = 0;
        while (getline(streamData, s, ',')) {
            if (i++ == 9){
                lines.push_back(s);
                break;
            }
        }
    }
    data_in.close();
    return lines;
}

std::vector<std::string> read_db_x() {
    std::string line;

    std::vector<std::string> lines;
    std::string path = "extracted/";

    std::string names_file = "db_x_files.txt";
    std::string curr_fname;
    std::ifstream fname_in(names_file);
    if (!fname_in.is_open()) {
        std::cerr << "Could not open the file - '" << names_file << "'" << std::endl;
        return lines;
    }
    while (getline(fname_in, curr_fname)){
        std::string data_file= path+curr_fname;
        std::ifstream data_in(data_file);
        if (!data_in.is_open()) {
            std::cerr << "Could not open the file - '" << data_file << "'" << std::endl;
            return lines;
        }
        while (getline(data_in, line)){
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            if (line.size() > 2)
                lines.push_back(line);
        }
        data_in.close();
    }
    fname_in.close();

    return lines;
}


int main(int argc, char** argv) {

    std::string line;
    // read all regexes
    std::vector<std::string> regexes;
    std::string reg_file = "../BLARE_DATA/regexes_traffic.txt";

    std::ifstream reg_in(reg_file);
    if (!reg_in.is_open()) {
        std::cerr << "Could not open the file - '" << reg_file << "'" << std::endl;
        return EXIT_FAILURE;
    }
    while (getline(reg_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        regexes.push_back(line);
    }
    reg_in.close();


    // auto lines = read_sys_y();
    auto lines = read_traffic();
    // auto lines = read_db_x();

    std::cout << lines.size() <<  std::endl;

    int num_repeat = 10;

    std::ofstream r_file("boost_traffic.csv", std::ofstream::out);
    r_file << "regex\ttrue_strategy\tave_stratey\tmid_ave_strategy\tnum_wrong\tblare_time\tmid_blare_time\tmulti_split_time\tmid_multi_split_time\tsm_time\tmid_sm_time\tdirect_time\tmid_direct_time\tmatch_num_blare\tmatch_num_multi_split\tmatch_num_split\tmatch_num_direct" << std::endl;

    for (const std::string & r : regexes) {
        std::vector<double> elapsed_time_blare;
        std::vector<double> elapsed_time_direct;
        std::vector<double> elapsed_time_split;
        std::vector<double> elapsed_time_multi;
        std::vector<double> strategies;
        int match_count_blare;
        int match_count_direct;
        int match_count_split;
        int match_count_multi;
        for (int i = 0; i < num_repeat; i++) {
            std::pair<double, int> result_direct = BoostFullAll(lines, r);
            std::pair<double, int> result_split = SplitMatchAll(lines, r);
            std::tuple<double, int, unsigned int> result_blare = Blare(lines, r);
            std::pair<double, int> result_multi = MultiSplitMatchTest(lines, r);

            elapsed_time_blare.push_back(std::get<0>(result_blare));
            strategies.push_back(std::get<2>(result_blare));
            elapsed_time_direct.push_back(result_direct.first);
            elapsed_time_split.push_back(result_split.first);
            elapsed_time_multi.push_back(result_multi.first);

            match_count_blare = std::get<1>(result_blare);
            match_count_direct = result_direct.second;
            match_count_split = result_split.second;
            match_count_multi = result_multi.second;
        }

        std::sort(elapsed_time_direct.begin(), elapsed_time_direct.end());
        auto ave_direct = std::accumulate(elapsed_time_direct.begin(), elapsed_time_direct.end(), 0.0) / elapsed_time_blare.size();
        auto mid_ave_direct = std::accumulate(elapsed_time_direct.begin()+1, elapsed_time_direct.end()-1, 0.0) / (num_repeat-2);

        std::sort(elapsed_time_multi.begin(), elapsed_time_multi.end());
        auto ave_multi = std::accumulate(elapsed_time_multi.begin(), elapsed_time_multi.end(), 0.0) / elapsed_time_multi.size();
        auto mid_ave_multi = std::accumulate(elapsed_time_multi.begin()+1, elapsed_time_multi.end()-1, 0.0) / (num_repeat-2);
        
        std::sort(elapsed_time_split.begin(), elapsed_time_split.end());
        auto ave_split = std::accumulate(elapsed_time_split.begin(), elapsed_time_split.end(), 0.0) / elapsed_time_multi.size();        
        auto mid_ave_split = std::accumulate(elapsed_time_split.begin()+1, elapsed_time_split.end()-1, 0.0) / (num_repeat-2);
        
        std::sort(elapsed_time_blare.begin(), elapsed_time_blare.end());
        std::sort(strategies.begin(), strategies.end());
        auto ave_blare = std::accumulate(elapsed_time_blare.begin(), elapsed_time_blare.end(), 0.0) / elapsed_time_blare.size();
        auto mid_ave_blare = std::accumulate(elapsed_time_blare.begin()+1, elapsed_time_blare.end()-1, 0.0) / (elapsed_time_blare.size()-2);

        auto ave_stratgy =  std::accumulate(strategies.begin(), strategies.end(), 0.0) / strategies.size();
        auto mid_ave_strategy = std::accumulate(strategies.begin()+1, strategies.end()-1, 0.0) / (strategies.size()-2);
        auto true_strategy = argmin(std::vector<double>{mid_ave_split, mid_ave_multi, mid_ave_direct});

        auto strategy_diff = std::abs(true_strategy-ave_stratgy) * num_repeat;

        std::cout << "Blare: " << ave_blare << " " << match_count_blare << std::endl;
        std::cout << "Direct: " << ave_direct << " " << match_count_direct << std::endl;
        std::cout << "SplitM: " << ave_split << " " << match_count_split << std::endl;
        std::cout << "MultiSplit: " << ave_multi << " " << match_count_multi << std::endl;

        r_file << r << "\t";
        r_file << true_strategy << "\t"  << ave_stratgy << "\t" << mid_ave_strategy << "\t" << strategy_diff << "\t";
        r_file << ave_blare << "\t" << mid_ave_blare << "\t";
        r_file << ave_multi << "\t" << mid_ave_multi << "\t";
        r_file << ave_split << "\t" << mid_ave_split << "\t";
        r_file << ave_direct << "\t" << mid_ave_direct << "\t";
        r_file << match_count_blare << "\t" << match_count_multi << "\t" << match_count_split << "\t" << match_count_direct << std::endl;
    }
    r_file.close();    
}