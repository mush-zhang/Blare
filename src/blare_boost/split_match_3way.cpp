#include <chrono>

#include <boost/random.hpp>
#include <boost/regex.hpp>

#include <misc/misc.hpp>
#include <misc/split_regex.hpp>
#include <blare_boost/split_match_3way.hpp>

std::pair<double, int> SplitMatch3WayBoost (const std::vector<std::string> & lines, const std::string & reg_string) {
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
    return std::make_pair(elapsed_seconds.count(), count);
}
