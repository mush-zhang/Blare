#include <chrono>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_pcre2/split_match_3way.hpp>

#include "jpcre2.hpp"

typedef jpcre2::select<char> jp;

std::pair<double, int> SplitMatch3WayPCRE2 (const std::vector<std::string> & lines, std::string reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    auto r = split_regex(reg_string);

    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    jp::VecNum vec_num;

    if (std::get<1>(r).empty()) {
        for (const auto & line : lines) {
            count += line.find(prefix) != std::string::npos;
        }
    } else if (prefix.empty()) {
        if (suffix.empty()) {
            jp::Regex re(std::get<1>(r), jpcre2::JIT_COMPILE);
            jp::RegexMatch rm(&re);

            for (const auto & line : lines) {
                count += rm.setSubject(line).setNumberedSubstringVector(&vec_num).match();
            }
        } else {
            jp::Regex re(std::get<1>(r), PCRE2_ENDANCHORED, jpcre2::JIT_COMPILE);
            jp::RegexMatch rm(&re);
            for (const auto & line : lines) {
                std::size_t pos = 0;
                while ((pos = line.find(suffix, pos)) != std::string::npos) {
                    std::string curr_in = line.substr(0, pos); 
                    if (rm.setSubject(curr_in).setNumberedSubstringVector(&vec_num).match()) {
                        count++;
                        break;
                    }
                    pos++;
                }
            }    
        }
    } else if (suffix.empty()) {
        jp::Regex re(std::get<1>(r), PCRE2_ANCHORED, jpcre2::JIT_COMPILE);
        jp::RegexMatch rm(&re);
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.find(prefix, pos)) != std::string::npos) {
                // for accuracy, should use line = line.substr(pos+1) next time
                std::string curr_in = line.substr(pos + prefix.length()); 
                if (rm.setSubject(curr_in).setNumberedSubstringVector(&vec_num).match()) {
                    count++;
                    break;
                }
                pos++;
            }                       
        }   
    } else {
        jp::Regex re(std::get<1>(r), PCRE2_ANCHORED | PCRE2_ENDANCHORED, jpcre2::JIT_COMPILE);
        jp::RegexMatch rm(&re);
        for (const auto & line : lines) {
            
            std::size_t pos = 0;
            while ((pos = line.find(prefix, pos)) != std::string::npos) {
                std::size_t reg_start_pos = pos + prefix.length();
                std::size_t reg_end_pos = reg_start_pos;
                while ((reg_end_pos = line.find(suffix, reg_end_pos)) != std::string::npos) {
                    std::string curr_in = line.substr(reg_start_pos, reg_end_pos - reg_start_pos ); 
                    
                    if (rm.setSubject(curr_in).setNumberedSubstringVector(&vec_num).match()) {
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
    return std::make_pair(elapsed_seconds.count(), count);
}