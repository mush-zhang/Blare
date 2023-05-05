#include <unicode/regex.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

#include <blare_icu/unicode_misc/regex_split.hpp>

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