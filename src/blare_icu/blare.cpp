#include <chrono>
#include <ctime> // seeding random number generators

#include <boost/random.hpp>

#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>

#include <misc/misc.hpp>
#include <blare_icu/unicode_misc/split_regex.hpp>
#include <blare_icu/blare.hpp>

using namespace icu_72;

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
                    prefix_idx = reg_idx;
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
    UErrorCode status = U_ZERO_ERROR;
    matcher->reset(line);
    return matcher->find(status);
}

std::tuple<double, int, unsigned int> BlareICU (const std::vector<UnicodeString> & lines, UnicodeString reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;
    unsigned int idx = 0;
    UErrorCode status = U_ZERO_ERROR;

    // pre_compile regs
    RegexPattern* reg_full = RegexPattern::compile(reg_string, 0, status);
    RegexMatcher* matcher_full = reg_full->matcher(status);

    auto r = split_unicode_regex(reg_string);
    RegexPattern* reg_suffix = RegexPattern::compile(std::get<1>(r), 0, status);
    RegexMatcher* matcher_suffix = reg_suffix->matcher(status);

    auto r_multi = split_unicode_regex_multi(reg_string);
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

    auto arm_num = 3;
    std::vector<int> pred_results{0, 0, 0};

    size_t skip_size = kSkipSize;
    size_t iteration_num = kEnembleNum;
    size_t sample_size = lines.size() / kSampleDivisor;

    boost::random::mt19937 gen(static_cast<std::uint32_t>(std::time(0)));
    boost::random::uniform_int_distribution<> dist{0, 2};

    std::vector<size_t> prev_prefix_pos(prefixes.size(), 0);
    
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

    for (; idx < skip_size; idx++) {
        switch(dist(gen)) {
            case ARM::kSplitMatch: count += SplitMatchSingle(lines[idx], matcher_suffix, r); break;
            case ARM::kMultiMatch: count += MultiMatchSingle(lines[idx], matchers, matcher0, prefixes, regs, prefix_first, prev_prefix_pos); break;
            case ARM::kDirectMatch: count += FullMatchSingle(lines[idx], matcher_full); break;
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
                case ARM::kSplitMatch: count += SplitMatchSingle(lines[idx], matcher_suffix, r); break;
                case ARM::kMultiMatch: count += MultiMatchSingle(lines[idx], matchers, matcher0, prefixes, regs, prefix_first, prev_prefix_pos); break;
                case ARM::kDirectMatch: count += FullMatchSingle(lines[idx], matcher_full); break;
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
        if (pred_results[winning_strategy] >= iteration_num*1.0 / arm_num) 
            break;
    }

    ARM_DECIDED:;

    auto chosen_bandit = argmax(pred_results);
    switch(chosen_bandit) {
        case ARM::kSplitMatch: {
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
        case ARM::kMultiMatch: {
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
                                prefix_idx = reg_idx;
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
        case ARM::kDirectMatch: {
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
    return std::make_tuple(elapsed_seconds.count(), count, chosen_bandit);
}