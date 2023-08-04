# BLAzingly Regular Expression

BLARE is a regular expression matching framework that decomposes regular expressions into components and uses an adaptive runtime evaluation plan to speed up the evaluation.

BLARE is modular and can be built on top of any existing regex library. Currently we have example BLARE implementation on 4 commonly used regex libraries: RE2, PCRE2, Boost Regex, and ICU Regex.

The paper is available: [**BLARE**](https://github.com/mush-zhang/Blare/tree/main/BLARE.pdf)

## Prerequisites

BLARE is implemented in C++, and we provide cmake file for building the project with the external dependencies excluding `g++`, `cmake`, and `Boost`. Make sure you have `g++` and `cmake` in you system. For Ubuntu as an example, you can do

```bash
sudo apt update && sudo apt upgrade
```

- g++ (version 8.4.0 or higher)
  
    ```bash
    sudo apt install build-essential
    ```

- cmake (version 3.14 or higher)

    ```bash
    sudo apt install cmake
    ```

- Boost Library (version 1.65.1.0 or higher)
  
    ```bash
    sudo apt-get install libboost-all-dev
    ```

- Git-LFS

    ```bash
    sudo apt-get install git-lfs
    ```

Make sure to check if the version satifies the requirement by

```bash
g++ --version
cmake --version
dpkg -s libboost-all-dev | grep 'Version'
```

## Instruction

We evaluate the performance of BLARE on two production workloads and one open-sourced workload. We have included the December 21st version of the open-source workload: [US Accident Dataset](https://www.kaggle.com/datasets/sobhanmoosavi/us-accidents) and the regular expressions in the repository. The newest version of the dataset has different format and may not work with the hard-coded csv parsing logic in the original codebase; consider modifiy the `read_traffic` function in the code that you will be running. To use the original version of the dataset, install `git lfs` then run

```bash
git lfs pull
```

to retrieve the dataset.

The code in the root directory is under continuous developement, and may not produce results identical to that in the BLARE paper. To reproduce most accurate results in the paper, compile and run the original experiment code in [**BLARE_CODE** folder](https://github.com/mush-zhang/Blare/tree/main/original_codebase/BLARE_CODE). The instruction for compilation and running is in the [**original_codebase** folder](https://github.com/mush-zhang/Blare/tree/main/original_codebase)

To build BLARE and experiments that can be run on customized workloads, follow the commands below:

```bash
mkdir build && cd build
cmake ..
make
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${PWD}/_deps/ICU-build/lib
```

Run BLARE with

```bash
cd src
./blare regex_lib_name input_regex_file input_data_file [output_file]
```

Run specific experiments comparing BLARE with underlying regex libraries, use

```bash
cd experiments
./[base_regex_library]_expr output_file [-n num_repeat] [-r input_regex_file] [-d input_data_file]
```

Run original code comparing BLARE, 3-Way-Split, Multi-Way-Split, with underlying regex libraries on the US-Accident dataset, use

```bash
cd original_codebase/BLARE_CODE
./blare_[base_regex_library]
```
