#include <iostream>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <string>
#include <string_view>
#include <sstream>
#include <cstring>
#include <algorithm>

#include <blare_boost/blare.hpp>
#include <blare_icu/blare.hpp>
#include <blare_pcre2/blare.hpp>
#include <blare_re2/blare.hpp>

inline constexpr const char * kRE2 = "RE2";
inline constexpr const char * kPCRE2 = "PCRE2";
inline constexpr const char * kICU = "ICU";
inline constexpr const char * kBoost = "BOOST";

inline constexpr std::string_view kUsage = "usage:  \n\
    main.o regex_lib_name input_regex_file input_data_file [output_file] \n\
    \t regex_lib_name:   Name of the base regex matching library used by BLARE. \n\
    \t                 Options available are 'RE2', 'PCRE2', 'ICU', 'BOOST'. \n\
    \t input_regex_file: Path to the list of regex queries. \n\
    \t                   Each line of the file is considered a regex query. \n\
    \t input_data_file:  Path to the list of data to be queried upon. \n\
    \t                   Each line of the file is considered an individual (log) line. \n\
    \t [output_file]:    Path to output file.";

inline constexpr std::string_view kHeader = "regex\ttime(s)\tnum_match";

int main(int argc, char** argv) {
    
    // arg[1]: base regex library
    // arg[2]: input regex file path (each regex in a line)
    // arg[3]: input data file path
    // arg[4]: output data file path

    if (argc < 4) {
        if (argc != 2 || (std::strcmp(argv[1], "--help")!=0 && std::strcmp(argv[1], "-h")!=0)) {
            std::cerr << "BLARE: arguments missing." << std::endl;
            std::cerr << kUsage << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << kUsage << std::endl;
            return EXIT_SUCCESS;
        }
    }

    if (std::strcmp(argv[1], kRE2)!=0 && std::strcmp(argv[1], kPCRE2)!=0 && std::strcmp(argv[1], kICU)!=0 && std::strcmp(argv[1], kBoost)!=0) {
        std::cerr << "BLARE: unknow base regex library." << std::endl;
        std::cerr << kUsage << std::endl;
        return EXIT_FAILURE;
    }

    std::string line;

    std::vector<std::string> regexes;
    std::ifstream reg_in(argv[2]);
    if (!reg_in.is_open()) {
        std::cerr << "BLARE: could not open regex file '" << argv[2] << "'" << std::endl;
        std::cerr << kUsage << std::endl;
        return EXIT_FAILURE;
    }
    while (getline(reg_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        regexes.push_back(line);
    }
    reg_in.close();

    std::vector<std::string> lines;
    std::ifstream data_in(argv[3]);
    if (!data_in.is_open()) {
        std::cerr << "BLARE: could not open data file '" << argv[3] << "'" << std::endl;
        std::cerr << kUsage << std::endl;
        return EXIT_FAILURE;
    }
    while (getline(data_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(line);
    }
    data_in.close();    

    std::vector<std::string_view> output_lines;
    output_lines.push_back(kHeader);

    for (const std::string & r : regexes) {
        std::tuple<double, int, unsigned int> result;
        if (std::strcmp(argv[1], kRE2)==0) {
            result = BlareRE2(lines, r);
        } else if (std::strcmp(argv[1], kPCRE2)==0) {
            result = BlarePCRE2(lines, r);
        } else if (std::strcmp(argv[1], kICU)==0) {
            result = BlareICU(lines, r);
        } else if (std::strcmp(argv[1], kBoost)==0) {
            result = BlareBoost(lines, r);
        }
        std::ostringstream ss;
        ss << r << "\t" << std::get<0>(result) << "\t" << std::get<1>(result);
        output_lines.push_back(ss.str());
    }

    if (argc == 5) {
        std::ofstream r_file(argv[4], std::ofstream::out);
        if (!r_file.is_open()) {
            std::cerr << "BLARE: could not open output file '" << argv[4] << "'" << std::endl;
            std::cerr << kUsage << std::endl;
            return EXIT_FAILURE;
        }
        for (const auto & l : output_lines) {
            r_file << l << std::endl;
        }
        r_file.close();        
    }

    for (const auto & l : output_lines) {
        std::cout << l << std::endl;
    }
    
    return EXIT_SUCCESS;
}
