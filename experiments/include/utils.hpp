#include <string>
#include <string_view>
#include <functional>

#ifdef ICU_FLAG
    #include <unicode/unistr.h>
#endif

inline constexpr const char * kRegexDefault = "../data/regexes_traffic.txt";
inline constexpr const char * kDataDefault = "../data/US_Accidents_Dec21_updated.csv";

inline constexpr std::string_view kUsage = "usage:  \n\
    [base_regex_library]_expr.o output_file [-h] [-n num_repeat] [-r input_regex_file] [-d input_data_file] \n\
    \t output_file:           Path to output file. \n\
    \t [-h]:                  Print usage, as what we are currently doing. :) \n\
    \t [-n num_repeat]:       Number of experiment repititions. Use 10 by default. \n\
    \t [-r input_regex_file]: Path to the list of regex queries. \n\
    \t                        Each line of the file is considered a regex query. \n\
    \t                        Use ../data/regexes_traffic.txt by default. \n\
    \t [-d input_data_file]:  Path to the list of data to be queried upon. \n\
    \t                        Each line of the file is considered an individual (log) line. \n\
    \t                        Use ../data/US_Accidents_Dec21_updated.csv by default.";

inline constexpr std::string_view kHeader = "regex\t\
    true_strategy\tave_stratey\tmid_ave_strategy\tnum_wrong\t\
    blare_time\tmid_blare_time\t\
    multi_split_time\tmid_multi_split_time\t\
    3way_split_time\tmid_3way_split_time\t\
    direct_time\tmid_direct_time\t\
    match_num_blare\tmatch_num_multi_split\tmatch_num_3way_split\tmatch_num_direct";

// argument parsing from https://stackoverflow.com/a/868894
char * getCmdOption(char ** begin, char ** end, const std::string & option);

bool cmdOptionExists(char** begin, char** end, const std::string& option);

template<class T>
std::pair<T, T> getStats(std::vector<T> & arr);

std::vector<std::string> readTraffic();

int parseArgs(int argc, char** argv, int * num_repeat, std::string * input_regex_file, std::string * input_data_file);

std::vector<std::string> readDataIn(const std::string & file_type, const std::string & infile_name);

template<class T>
void experiment(std::ofstream & r_file, 
    const std::vector<T> & regexes, const std::vector<T> &lines, int num_repeat,
    std::function<std::pair<double, int>(const std::vector<T> &, const T &)> SplitMatchMultiWay,
    std::function<std::tuple<double, int, unsigned int>(const std::vector<T> &, const T &)> Blare,
    std::function<std::pair<double, int>(const std::vector<T> &, const T &)> SplitMatch3Way,
    std::function<std::pair<double, int>(const std::vector<T> &, const T &)> DirectMatch);