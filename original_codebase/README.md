# BLAzingly Regular Expression

BLARE is a regular expression matching framework that decomposes regular expressions into components and uses an adaptive runtime evaluation plan to speed up the evaluation.

BLARE is modular and can be built on top of any existing regex library. Currently we have example BLARE implementation on 4 commonly used regex libraries: RE2, PCRE2, Boost Regex, and ICU Regex.

This folder contains the original experiment scripts for the paper.

## Prerequisites

BLARE is implemented in C++. We use the Boost Random Number Library for random number generating support in BLARE. These 2 are the only dependencies of BLARE framework.

- g++ (version 8.4.0 or higher)
  
    ```bash
    sudo apt install build-essential
    ```

- Boost Library
  
    ```bash
    sudo apt-get install libboost-all-dev
    ```

- Base Regex Libraries
  
User can install the base regular expression matching library and adapt BLARE code on top of it. We included implementation of BLARE on top of several popular regex libraries. To run the sample experiment scripts with desired base regex library, you can install the regex libraries as followed:

- Google-RE2

    Please follow the instruction in [RE2 Github Repository](https://github.com/google/re2).

- PCRE2
  - PCRE2 library

    ```bash
    sudo apt-get install pcre2-utils
    ```

  - jPCRE2 (a C++ wrapper of PCRE2)

    Please follow the instruction in [jPCRE2 Github Repository](https://github.com/jpcre2/jpcre2).

- Boost.Regex should have already been installed.
- ICU Regex
  
    Please download [ICU4C 72 release](https://github.com/unicode-org/icu/releases/tag/release-72-1).

    ```bash
    tar xvfz icu4c-64_2-src.tgz
    cd icu/source
    ./configure --prefix=/usr/local/icu4c/64_2 --enable-icu-config
    make
    make install
    ```

## Instruction

We evaluate the performance of BLARE on two production workloads and one open-sourced workload. We have included the open-source workload: [US Accident Dataset](https://www.kaggle.com/datasets/sobhanmoosavi/us-accidents) and the regular expressions in the repository.


Compile and run the original experiment scripts for the paper with the following commands:

- BLARE on Google-RE2
  
    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_re2.cpp -L/usr/local/lib/ -lre2 -lstdc++fs -o blare_re2.o
    ./blare_re2.o
    ```

    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_re2_longest.cpp -L/usr/local/lib/ -lre2 -lstdc++fs -o blare_re2_longest.o
    ./blare_re2_longest.o
    ```

    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_re2_countAll.cpp -L/usr/local/lib/ -lre2 -lstdc++fs -o blare_re2_countAll.o
    ./blare_re2_countAll.o
    ```

    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_re2_4arms.cpp -L/usr/local/lib/ -lre2 -lstdc++fs -o blare_re2_4arms.o
    ./blare_re2_4arms.o
    ```

- BLARE on PCRE2

    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_pcre2.cpp -L/usr/local/lib/ -lstdc++fs \
        -lpcre2-8 -lpcre2-16 -lpcre2-32 -o blare_pcre2.o
    ./blare_pcre2.o
    ```

- BLARE on Boost.Regex

    ```bash
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_boost.cpp -L/usr/local/lib/ -lstdc++fs -lboost_regex -o blare_boost.o
    ./blare_boost.o
    ```

- BLARE on ICU Regex

    ```bash
    ulimit -s unlimited
    g++ -O3 -std=c++17 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
        -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread \
        blare_icu.cpp -L/usr/local/lib/ -lstdc++fs \
        `pkg-config --libs --cflags icu-i18n icu-uc icu-io` -o blare_icu.o
    ./blare_icu.o
    ```
