#include <algorithm>

#include <misc/split_regex.hpp>

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
