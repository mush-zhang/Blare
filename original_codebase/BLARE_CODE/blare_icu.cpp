#include <iostream>
#include <fstream>
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

#include <boost/random.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>


// #include <unicode/uregex.h>
#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

// Define base_generator as a Mersenne Twister. This is needed only to make the
// code a bit less verbose.
typedef boost::mt19937 base_generator;
using namespace icu_72;

bool char_escaped(const UnicodeString &line, std::size_t pos) {
    auto temp_pos = pos;
    int num_escaped = 0;
    // count number of escape characters before the char
    while (temp_pos > 0) {
        if (line.charAt(--temp_pos) != '\\') 
            break;
        num_escaped++;
    }
    return num_escaped % 2 == 1;
}

std::tuple<UnicodeString, UnicodeString, UnicodeString> split_regex(const UnicodeString &line) {
    std::size_t pos = 0;
    UnicodeString prefix = line;
    UnicodeString regex = "";
    UnicodeString suffix = "";
    while ((pos = line.indexOf("(", pos)) != -1) {
        if (!char_escaped(line, pos)) {
            std::size_t reg_start_pos = pos;
            std::size_t reg_end_pos = pos+1;
            while ((pos = line.indexOf(")", pos+1)) != -1) {
                if (!char_escaped(line, pos)) {
                    reg_end_pos = pos;
                }
                pos++;
            }
            prefix = line.tempSubString(0, reg_start_pos);
            regex = line.tempSubString(reg_start_pos, reg_end_pos+1-reg_start_pos);
            suffix = line.tempSubString(reg_end_pos+1);
            
            break;
        }
        pos++;
    }
    prefix.findAndReplace("\\", "");
    suffix.findAndReplace("\\", "");
    return std::make_tuple(prefix, regex, suffix);
}

std::tuple<std::vector<UnicodeString>, std::vector<UnicodeString>, bool> split_regex_multi(const UnicodeString &line) {
    std::size_t pos = 0;
    std::size_t prev_pos = 0;
    int pos2 = -1;
    std::vector<UnicodeString> const_strings;
    std::vector<UnicodeString> regexes;
    UnicodeString prefix = line;
    UnicodeString suffix = "";
    bool prefix_first = true;
    while ((pos = line.indexOf("(", pos)) != -1) {
        if (!char_escaped(line, pos)) {
            prefix = line.tempSubString(prev_pos, pos-prev_pos);
            if (prev_pos == 0 && pos == 0) prefix_first = false;

            pos2 = pos;
            while ((pos2 = line.indexOf(")", pos2)) != -1) {
                if (!char_escaped(line, pos2)) {
                    suffix = line.tempSubString(pos, pos2 + 1 -pos);
                    break;
                }
                pos2++;
            }
            
            if (pos2 == -1 && suffix.isEmpty()) {
                suffix = line.tempSubString(pos);
                pos2 = line.length();
            } else {
                pos2++; 
            }
            regexes.push_back(suffix);
            if (!prefix.isEmpty()) {
                prefix.findAndReplace("\\", "");
                const_strings.push_back(prefix);
            }
            
            prev_pos = pos2;
            pos = pos2;
        } else {
            pos++;
        }
    }
    prefix = line.tempSubString(pos2);
    if (!prefix.isEmpty()) {
        prefix.findAndReplace("\\", "");
        const_strings.push_back(prefix);
    }
    return std::tuple<std::vector<UnicodeString>, std::vector<UnicodeString>, bool>(const_strings, regexes, prefix_first);
}

bool MultiMatchSingle (const UnicodeString & line, const std::vector<std::shared_ptr<RegexMatcher>> & matchers, 
                       std::shared_ptr<RegexMatcher> const & matcher0, const std::vector<UnicodeString> prefixes, 
                       const std::vector<UnicodeString> & regs, bool prefix_first, std::vector<size_t> & prev_prefix_pos) {
    UErrorCode status = U_ZERO_ERROR;
    bool match = false;
    if (regs.empty()) {
        return line.indexOf(prefixes[0]) != -1;
    } else if (prefixes.empty()) {
        matcher0->reset(line);
        return matcher0->find(status);
    } else {
        size_t pos = 0;
        size_t curr_prefix_pos = 0;
        size_t prefix_idx = 0;
        size_t reg_idx = 0;
        MATCH_LOOP_SINGLE:
            for (; prefix_idx < prefixes.size(); ) {
                // find pos of prefix before reg
                if ((curr_prefix_pos = line.indexOf(prefixes[prefix_idx], pos)) == -1) {
                    if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                        goto CONTINUE_OUTER_SINGLE;
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
                    pos = prev_prefix_pos[reg_idx]+1;
                    prefix_idx = reg_idx+1;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            if (prefixes.size() == regs.size() && prefix_first) {
                auto curr = line.tempSubString(pos);
                matchers.back()->reset(curr);
                if (!matchers.back()->lookingAt(status)) {
                    prefix_idx = prev_prefix_pos.size() -1;
                    pos = prev_prefix_pos[prefix_idx]+1;
                    goto MATCH_LOOP_SINGLE;
                }
            }
            if (!prefix_first) {
                auto curr = line.tempSubString(0,prev_prefix_pos[0]);
                matcher0->reset(curr);
                if (!matcher0->find(status)){
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

bool SplitMatchSingle (const UnicodeString & line, RegexMatcher * const matcher, const std::tuple<UnicodeString, UnicodeString, UnicodeString> & r) {
    // std::cout << "split" << std::endl;

    UErrorCode status = U_ZERO_ERROR;

    bool match = false;
    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    if (std::get<1>(r).isEmpty()) {
        match = line.indexOf(prefix) != -1;
    } else if (prefix.isEmpty()) {
        if (suffix.isEmpty()) {
            matcher->reset(line);
            match = matcher->find(status);
        } else {
            std::size_t pos = 0;
            while ((pos = line.indexOf(suffix, pos)) != -1) {
                auto curr = line.tempSubString(0, pos);
                matcher->reset(curr);
                while (matcher->lookingAt(status)) {
                    if (matcher->end(status) == pos) {
                        match = true;
                        goto NEXT_LINE2;
                    } 
                }
                pos++;
            }
            NEXT_LINE2:;
        }
    } else if (suffix.isEmpty()) {
        std::size_t pos = 0;
        while ((pos = line.indexOf(prefix, pos)) != -1) {
            // for accuracy, should use line = line.tempSubString(pos+1) next time
            matcher->reset(line);
            if (matcher->lookingAt(pos + prefix.length(), status)) {
                match = true;
                break;
            }
            pos++;
        }                       
    } else {
        std::size_t pos = 0;
        while ((pos = line.indexOf(prefix, pos)) != -1) {
            std::size_t reg_start_pos = pos + prefix.length();
            std::size_t reg_end_pos = reg_start_pos;
            while ((reg_end_pos = line.indexOf(suffix, reg_end_pos)) != -1) {
                auto curr = line.tempSubString(reg_start_pos, reg_end_pos - reg_start_pos);
                matcher->reset(curr);
                if (matcher->matches(status)) {
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

bool FullMatchSingle (const UnicodeString & line, RegexMatcher * const matcher) {
    // std::cout << "full" << std::endl;

    UErrorCode status = U_ZERO_ERROR;
    matcher->reset(line);
    return matcher->find(status);
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

std::tuple<double, int, unsigned int> Blare (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    unsigned int idx = 0;
    UErrorCode status = U_ZERO_ERROR;

    // pre_compile regs
    RegexPattern* reg_full = RegexPattern::compile(reg_string, 0, status);
    RegexMatcher* matcher_full = reg_full->matcher(status);

    auto r = split_regex(reg_string);
    RegexPattern* reg_suffix = RegexPattern::compile(std::get<1>(r), 0, status);
    RegexMatcher* matcher_suffix = reg_suffix->matcher(status);

    auto r_multi = split_regex_multi(reg_string);
    std::vector<UnicodeString> prefixes = std::get<0>(r_multi);
    std::vector<UnicodeString> regs_temp = std::get<1>(r_multi);
    auto regs = std::get<1>(r_multi);
    bool prefix_first = std::get<2>(r_multi);

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
        switch(dist(gen)) {
            case 0: count += SplitMatchSingle(lines[idx], matcher_suffix, r); break;
            case 1: count += MultiMatchSingle(lines[idx], matchers, matcher0, prefixes, regs, prefix_first, prev_prefix_pos); break;
            case 2: count += FullMatchSingle(lines[idx], matcher_full); break;
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
                case 0: count += SplitMatchSingle(lines[idx], matcher_suffix, r); break;
                case 1: count += MultiMatchSingle(lines[idx], matchers, matcher0, prefixes, regs, prefix_first, prev_prefix_pos); break;
                case 2: count += FullMatchSingle(lines[idx], matcher_full); break;
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
        if (pred_results[winning_strategy] >= iteration_num*1.0 / arm_num) 
            break;
    }
    auto chosen_bandit = argmax(pred_results);
    switch(chosen_bandit) {
        case 0: {
            auto prefix = std::get<0>(r);
            auto suffix = std::get<2>(r);
            if (std::get<1>(r).isEmpty()) {
                for (; idx < lines.size(); idx++) {
                    count += lines[idx].indexOf(prefix) != -1;
                }
            } else if (prefix.isEmpty()) {
                if (suffix.isEmpty()) {
                    for (; idx < lines.size(); idx++) {
                        matcher_suffix->reset(lines[idx]);
                        count += matcher_suffix->find(status);
                    }
                } else {
                    for (; idx < lines.size(); idx++) {
                        std::size_t pos = 0;
                        while ((pos = lines[idx].indexOf(suffix, pos)) != -1) {
                            auto curr = lines[idx].tempSubString(0, pos);
                            matcher_suffix->reset(curr);
                            while (matcher_suffix->lookingAt(status)) {
                                if (matcher_suffix->end(status) == pos) {
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
                for (; idx < lines.size(); idx++) {
                    std::size_t pos = 0;
                    while ((pos = lines[idx].indexOf(prefix, pos)) != -1) {
                        matcher_suffix->reset(lines[idx]);
                        if (matcher_suffix->lookingAt(pos + prefix.length(), status)) {
                            count++;
                            break;
                        }
                        pos++;
                    }
                }  
            } else {
                for (; idx < lines.size(); idx++) {
                    std::size_t pos = 0;
                    while ((pos = lines[idx].indexOf(prefix, pos)) != -1) {
                        std::size_t reg_start_pos = pos + prefix.length();
                        std::size_t reg_end_pos = reg_start_pos;
                        while ((reg_end_pos = lines[idx].indexOf(suffix, reg_end_pos)) != -1) {
                            auto curr = lines[idx].tempSubString(reg_start_pos, reg_end_pos - reg_start_pos);
                            matcher_suffix->reset(curr);
                            if (matcher_suffix->matches(status)) {
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
                    count += lines[idx].indexOf(prefixes[0]) != -1;
                }
            } else if (prefixes.empty()) {
                RegexPattern* reg = RegexPattern::compile(regs[0], 0, status);
                RegexMatcher* matcher = reg->matcher(status);
                for (; idx < lines.size(); idx++) {
                    matcher->reset(lines[idx]);
                    count += matcher->find(status); 
                }
                delete matcher;
                delete reg;
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
                            if ((curr_prefix_pos = line.indexOf(prefixes[prefix_idx], pos)) == -1) {
                                if (prefix_idx == 0 || prev_prefix_pos[prefix_idx] == 0 || reg_idx == 0)
                                    goto CONTINUE_OUTER_BLARE;
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
                                pos = prev_prefix_pos[reg_idx]+1;
                                prefix_idx = reg_idx+1;
                                goto MATCH_LOOP_BLARE;
                            }
                        }
                        if (prefixes.size() == regs.size() && prefix_first) {
                            auto curr = line.tempSubString(pos);
                            matchers.back()->reset(curr);
                            if (!matchers.back()->lookingAt(status)) {
                                prefix_idx = prev_prefix_pos.size() -1;
                                pos = prev_prefix_pos[prefix_idx]+1;
                                goto MATCH_LOOP_BLARE;
                            } 
                        }
                        if (!prefix_first) {
                            auto curr = line.tempSubString(0,prev_prefix_pos[0]);
                            matcher0->reset(curr);
                            if (!matcher0->find(status)){
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
                matcher_full->reset(lines[idx]);
                count += matcher_full->find(status);
            } 
            break;
        }
    }

    delete matcher_suffix;
    delete reg_suffix;
    delete matcher_full;
    delete reg_full;
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << "Blare: " << count << std::endl;
    std::cout << "sample size: " << sample_size <<" chooses: " << chosen_bandit  << std::endl;
    return std::make_tuple(elapsed_seconds.count(), count, chosen_bandit);
}

std::pair<double, int> ICUFullAll (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;

    RegexPattern* reg = RegexPattern::compile(reg_string, 0, status);
    RegexMatcher* matcher = reg->matcher(status);

    for (const auto & line : lines) {
        matcher->reset(line);
        count += matcher->find(status);
    }
    delete matcher;
    delete reg;
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << "direct: ";
    std::cout << reg_string;
    std::cout << " " << count << std::endl;
    return std::make_pair(elapsed_seconds.count(), count);
}

std::pair<double, int> SplitMatchAll (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
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

    std::cout << "split: ";
    std::cout << reg_string;
    std::cout << " " << count << std::endl;

    return std::make_pair(elapsed_seconds.count(), count);
}

std::pair<double, int> MultiSplitMatchTest (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;

    auto r = split_regex_multi(reg_string);

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

                    // auto curr = UnicodeString(line, prev_prefix_end_pos, prev_prefix_pos[reg_idx+1] - prev_prefix_end_pos);
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
    
    for (const auto & p : prefixes) {
        std::cout << "[";
        std::cout << p;
        std::cout << "]\t";  
    }
    std::cout << std::endl;

    std::cout << "Multi Split ";
    std::cout << reg_string;
    std::cout << " " << count << std::endl;

    return std::make_pair(elapsed_seconds.count(), count);
}

std::vector<UnicodeString> read_sys_y() {
    std::string line;
    std::vector<UnicodeString> lines;
    std::string data_file = "tagged_data.csv";
    std::cout << "reading: " << data_file <<  std::endl;

    std::ifstream data_in(data_file);
    if (!data_in.is_open()) {
        std::cerr << "Could not open the file - '" << data_file << "'" << std::endl;
        return lines;
    }
    int i = 0;
    while (getline(data_in, line) && i < 30000000){
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
        UnicodeString uline = UnicodeString::fromUTF8(StringPiece(curr));
        lines.push_back(uline);
    }
    data_in.close();
    return lines;
}

std::vector<UnicodeString> read_traffic() {
    std::string line;
    std::vector<UnicodeString> lines;
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
                UnicodeString uline = UnicodeString::fromUTF8(StringPiece(s));
                lines.push_back(uline);
                break;
            }
        }
    }
    data_in.close();
    return lines;
}

std::vector<UnicodeString> read_db_x() {
    std::string line;

    std::vector<UnicodeString> lines;
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
            if (line.size() > 2) {
                UnicodeString uline = UnicodeString::fromUTF8(StringPiece(line));
                lines.push_back(uline);
            }
        }
        data_in.close();
    }
    fname_in.close();

    return lines;
}


int main(int argc, char** argv) {

    std::string line;
    // read all regexes
    std::vector<UnicodeString> regexes;
    std::string reg_file = "../BLARE_DATA/regexes_traffic.txt";

    std::ifstream reg_in(reg_file);
    if (!reg_in.is_open()) {
        std::cerr << "Could not open the file - '" << reg_file << "'" << std::endl;
        return EXIT_FAILURE;
    }
    while (getline(reg_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        UnicodeString uline = UnicodeString::fromUTF8(StringPiece(line));

        regexes.push_back(uline);
    }
    reg_in.close();


    // auto lines = read_sys_y();
    auto lines = read_traffic();
    // auto lines = read_db_x();

    std::cout << lines.size() <<  std::endl;

    int num_repeat = 10;

    std::ofstream r_file("icu_traffic.csv", std::ofstream::out);
    r_file << "regex\ttrue_strategy\tave_stratey\tmid_ave_strategy\tnum_wrong\tblare_time\tmid_blare_time\tmulti_split_time\tmid_multi_split_time\tsm_time\tmid_sm_time\tdirect_time\tmid_direct_time\tmatch_num_blare\tmatch_num_multi_split\tmatch_num_split\tmatch_num_direct" << std::endl;

    for (const UnicodeString & r : regexes) {
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
            std::pair<double, int> result_multi = MultiSplitMatchTest(lines, r);
            std::tuple<double, int, unsigned int> result_blare = Blare(lines, r);
            std::pair<double, int> result_direct = ICUFullAll(lines, r);
            std::pair<double, int> result_split = SplitMatchAll(lines, r);

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

        r_file << r; 
        r_file << "\t";
        r_file << true_strategy << "\t"  << ave_stratgy << "\t" << mid_ave_strategy << "\t" << strategy_diff << "\t";
        r_file << ave_blare << "\t" << mid_ave_blare << "\t";
        r_file << ave_multi << "\t" << mid_ave_multi << "\t";
        r_file << ave_split << "\t" << mid_ave_split << "\t";
        r_file << ave_direct << "\t" << mid_ave_direct << "\t";
        r_file << match_count_blare << "\t" << match_count_multi << "\t" << match_count_split << "\t" << match_count_direct << std::endl;
    }
    r_file.close();    
}
