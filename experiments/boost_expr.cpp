#include <iostream>
#include <fstream>

#include <blare_boost/blare.hpp>
#include <blare_boost/direct_match.hpp>
#include <blare_boost/split_match_3way.hpp>
#include <blare_boost/split_match_multiway.hpp>

#include <utils.hpp>
#include <misc/misc.hpp>

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

    auto regexes = readDataIn("regex", input_regex_file);
    if (regexes.empty()) {
        return EXIT_FAILURE;
    }

    std::vector<std::string> lines;
    if (input_data_file.empty()) {
        lines = readTraffic();
    } else {
        lines = readDataIn("data", input_data_file);
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

    experiment<std::string>(r_file, regexes, lines, num_repeat, 
        & SplitMatchMultiWayBoost, & BlareBoost, 
        & SplitMatch3WayBoost, & DirectMatchBoost);

    r_file.close(); 
}
