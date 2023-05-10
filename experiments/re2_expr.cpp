#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <string>
#include <string_view>
#include <sstream>
#include <blare_re2/blare.hpp>
#include <blare_re2/direct_match.hpp>
#include <blare_re2/split_match_3way.hpp>
#include <blare_re2/split_match_multiway.hpp>
#include <misc/misc.hpp>

inline constexpr const char * kRegexDefault = "../data/regexes_traffic.txt";
inline constexpr const char * kDataDefault = "../data/US_Accidents_Dec21_updated.csv";

inline constexpr std::string_view kUsage = "usage:  \
    re2_expr.o output_file [-h] [-n num_repeat] [-r input_regex_file] [-d input_data_file] \
    \t output_file:           Path to output file. \
    \t [-h]:                  Print usage, as what we are currently doing. :) \
    \t [-n num_repeat]:       Number of experiment repititions. Use 10 by default. \
    \t [-r input_regex_file]: Path to the list of regex queries. \
    \t                        Each line of the file is considered a regex query. \
    \t                        Use ../data/regexes_traffic.txt by default. \
    \t [-d input_data_file]:  Path to the list of data to be queried upon. \
    \t                        Each line of the file is considered an individual (log) line. \
    \t                        Use ../data/US_Accidents_Dec21_updated.csv by default.";

inline constexpr std::string_view kHeader = "regex\t\
    true_strategy\tave_stratey\tmid_ave_strategy\tnum_wrong\t\
    blare_time\tmid_blare_time\t\
    multi_split_time\tmid_multi_split_time\t\
    3way_split_time\tmid_3way_split_time\t\
    direct_time\tmid_direct_time\t\
    match_num_blare\tmatch_num_multi_split\tmatch_num_3way_split\tmatch_num_direct";

// argument parsing from https://stackoverflow.com/a/868894
char * getCmdOption(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

template<class T>
std::pair<T, T> getStats(std::vector<T> & arr) {
    auto num_reps = arr.size();
    std::sort(arr.begin(), arr.end());
    auto ave= std::accumulate(arr.begin(), arr.end(), 0) / num_reps;
    auto trimmed_ave = ave;
    if (num_reps > 3) {
        trimmed_ave = std::accumulate(arr.begin()+1, arr.end()-1, 0) / (num_reps-2);
    }
    return std::make_pair(ave, trimmed_ave);
}

std::vector<std::string> readTraffic() {
    std::string line;
    std::vector<std::string> lines;
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
                lines.push_back(s);
                break;
            }
        }
    }
    data_in.close();
    return lines;
}

int main(int argc, char** argv) {
    // argument -h for help
    // argument should have -r -d -o for optional input regex file, input data file, and output file specification

    if(cmdOptionExists(argv, argv+argc, "-h")) {
        std::cout << kUsage << std::endl;
        return EXIT_SUCCESS;
    }

    if (argc < 2) {
        std::cerr << "Arguments missing: output file." << std::endl;
        std::cerr << kUsage << std::endl;
        return EXIT_FAILURE;
    }

    int num_repeat = 10;
    auto repeat_string = getCmdOption(argv, argv + argc, "-n");
    if (repeat_string) {
        num_repeat = std::stoi(repeat_string);
    }

    std::string line;

    std::string input_regex_file = kRegexDefault;
    auto input_regex_file_raw = getCmdOption(argv, argv + argc, "-r");
    if (input_regex_file_raw) {
        input_regex_file = input_regex_file_raw;
    }
    std::ifstream reg_in(input_regex_file);
    if (!reg_in.is_open()) {
        std::cerr << "Could not open regex file '" << input_regex_file << "'" << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<std::string> regexes;
    while (getline(reg_in, line)){
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        regexes.push_back(line);
    }
    reg_in.close();

    std::vector<std::string> lines;
    auto input_data_file_raw = getCmdOption(argv, argv + argc, "-d");
    if (!input_data_file_raw) {
        lines = readTraffic();
        if (lines.empty()) {
            return EXIT_FAILURE;
        }
    } else {
        std::ifstream data_in(input_data_file_raw);
        if (!data_in.is_open()) {
            std::cerr << "Could not open data file '" << input_data_file_raw << "'" << std::endl;
            return EXIT_FAILURE;
        }
        while (getline(data_in, line)){
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            lines.push_back(line);
        }
        data_in.close();
    }

    std::ofstream r_file(argv[1], std::ofstream::out);
    if (!r_file.is_open()) {
        std::cerr << "Could not open output file '" << argv[1] << "'" << std::endl;
        return EXIT_FAILURE;
    }
    r_file << kHeader << std::endl;

    for (const std::string & r : regexes) {
        std::vector<double> elapsed_time_blare;
        std::vector<double> elapsed_time_direct;
        std::vector<double> elapsed_time_split;
        std::vector<double> elapsed_time_multi;
        std::vector<int> strategies;
        int match_count_blare;
        int match_count_direct;
        int match_count_split;
        int match_count_multi;
        for (int i = 0; i < num_repeat; i++) {
            auto [dtime, dnum] = DirectMatchRe2(lines, r);
            auto [smtime, smnum] = SplitMatch3WayRe2(lines, r);
            auto [btime, bnum, bstrat] = BlareRE2(lines, r);
            auto [multime, mulnum] = SplitMatchMultiWayRe2(lines, r);

            elapsed_time_blare.push_back(btime);
            strategies.push_back(bstrat);
            elapsed_time_direct.push_back(dtime);
            elapsed_time_split.push_back(smtime);
            elapsed_time_multi.push_back(multime);

            match_count_blare = bnum;
            match_count_direct = dnum;
            match_count_split = smnum;
            match_count_multi = mulnum;
        }
        
        auto [ave_direct, mid_ave_direct] = getStats(elapsed_time_direct);
        auto [ave_multi, mid_ave_multi] = getStats(elapsed_time_multi);
        auto [ave_split, mid_ave_split] = getStats(elapsed_time_split);
        auto [ave_blare, mid_ave_blare] = getStats(elapsed_time_blare);
        auto [ave_stratgy, mid_ave_strategy] = getStats(strategies);

        std::vector<double> mid_times(3, 0);
        mid_times[ARM::kSplitMatch] = mid_ave_split;
        mid_times[ARM::kMultiMatch] = mid_ave_multi;
        mid_times[ARM::kDirectMatch] = mid_ave_direct;
        auto true_strategy = std::distance(mid_times.begin(), std::min_element(mid_times.begin(), mid_times.end()));
        auto strategy_diff = std::count(strategies.begin(), strategies.end(), true_strategy);

        r_file << r << "\t";
        r_file << true_strategy << "\t"  << ave_stratgy << "\t" << mid_ave_strategy << "\t" << strategy_diff << "\t";
        r_file << ave_blare << "\t" << mid_ave_blare << "\t";
        r_file << ave_multi << "\t" << mid_ave_multi << "\t";
        r_file << ave_split << "\t" << mid_ave_split << "\t";
        r_file << ave_direct << "\t" << mid_ave_direct << "\t";
        r_file << match_count_blare << "\t" << match_count_multi << "\t" << match_count_split << "\t" << match_count_direct << std::endl;
    }
    r_file.close(); 
}
