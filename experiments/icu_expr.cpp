#include <iostream>
#include <fstream>
#include <sstream>

#include <unicode/unistr.h>

#include <blare_icu/blare.hpp>
#include <blare_icu/direct_match.hpp>
#include <blare_icu/split_match_3way.hpp>
#include <blare_icu/split_match_multiway.hpp>

#include <utils.hpp>
#include <misc/misc.hpp>

std::vector<icu_72::UnicodeString> readDataInICU(const std::string & file_type, const std::string & infile_name) {
    std::vector<icu_72::UnicodeString> in_strings;
    std::ifstream data_in(infile_name);
    if (!data_in.is_open()) {
        std::cerr << "Could not open " << file_type << " file '" << infile_name << "'" << std::endl;
    } else {
        std::string line;
        while (getline(data_in, line)){
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            icu_72::UnicodeString uline = icu_72::UnicodeString::fromUTF8(icu_72::StringPiece(line));
            in_strings.push_back(uline);
        }
        data_in.close();
    }
    return in_strings;
}

std::vector<icu_72::UnicodeString> readTrafficICU() {
    std::string line;
    std::vector<icu_72::UnicodeString> lines;
    std::ifstream data_in(kDataDefault);
    if (!data_in.is_open()) {
        std::cerr << "Could not open data file '" << kDataDefault << "'" << std::endl;
        std::cerr << "Try downloading it first per instruction in ../data/dataset.txt" << std::endl;
        return lines;
    }
    while (getline(data_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        std::stringstream streamData(line);
        std::string s; 
        int i = 0;
        while (getline(streamData, s, ',')) {
            if (i++ == 9){
                icu_72::UnicodeString us = icu_72::UnicodeString::fromUTF8(icu_72::StringPiece(s));
                lines.push_back(us);
                break;
            }
        }
    }
    data_in.close();
    return lines;
}

int main(int argc, char** argv) {
    // argument -h for help
    if(cmdOptionExists(argv, argv+argc, "-h")) {
        std::cout << kUsage << std::endl;
        return EXIT_SUCCESS;
    }
    int num_repeat;
    std::string input_data_file, input_regex_file;

    int status = parseArgs(argc, argv, & num_repeat, & input_regex_file, & input_data_file);
    if (status == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    auto regexes = readDataInICU("regex", input_regex_file);
    if (regexes.empty()) {
        return EXIT_FAILURE;
    }

    std::vector<icu_72::UnicodeString> lines;
    if (input_data_file.empty()) {
        lines = readTrafficICU();
    } else {
        lines = readDataInICU("data", input_data_file);
    }
    if (lines.empty()) {
        return EXIT_FAILURE;
    }

    std::ofstream r_file(argv[1], std::ofstream::out);
    if (!r_file.is_open()) {
        std::cerr << "Could not open output file '" << argv[1] << "'" << std::endl;
        return EXIT_FAILURE;
    }
    r_file << kHeader << std::endl;

    experiment<icu_72::UnicodeString>(r_file, regexes, lines, num_repeat, & SplitMatchMultiWayICU, & BlareICU, & SplitMatch3WayICU, & DirectMatchICU);

    r_file.close(); 
}
