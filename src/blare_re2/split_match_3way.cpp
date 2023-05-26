#include <chrono>

#include <re2/re2.h>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_re2/split_match_3way.hpp>

std::pair<double, int> SplitMatch3WayRE2 (const std::vector<std::string> & lines, const std::string & reg_string) {
    auto start = std::chrono::high_resolution_clock::now();
    int count = 0;

    auto r = split_regex(reg_string);

    auto prefix = std::get<0>(r);
    auto suffix = std::get<2>(r);
    RE2 reg(std::get<1>(r));
    std::string sm;

    if (std::get<1>(r).empty()) {
        for (const auto & line : lines) {
            count += line.find(prefix) != std::string::npos;
        }
    } else if (prefix.empty()) {
        if (suffix.empty()) {
            for (const auto & line : lines) {
                count += RE2::PartialMatch(line, reg, &sm);
            }
        } else {
            for (const auto & line : lines) {
                std::size_t pos = 0;
                while ((pos = line.find(suffix, pos)) != std::string::npos) {
                    std::string curr_in = line.substr(0, pos); 
                    re2::StringPiece input(curr_in);
                    while (RE2::Consume(&input, reg, &sm)) {
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
        for (const auto & line : lines) {
            std::size_t pos = 0;
            while ((pos = line.find(prefix, pos)) != std::string::npos) {
                std::string curr_in = line.substr(pos + prefix.length()); 
                re2::StringPiece input(curr_in);
                if (RE2::Consume(&input, reg, &sm)) {
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
                    if (RE2::FullMatch(line.substr(reg_start_pos, reg_end_pos - reg_start_pos ), reg, &sm)) {
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