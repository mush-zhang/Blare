# BLAzingly Regular Expression

BLARE is a regular expression matching framework that decomposes regular expressions into components and uses an adaptive runtime evaluation plan to speed up the evaluation.

BLARE is modular and can be built on top of any existing regex library. Currently we have example BLARE implementation on 4 commonly used regex libraries: RE2, PCRE2, Boost Regex, and ICU Regex.

This folder contains the original experiment scripts for the paper.

## Prerequisites

Besides `g++`, `cmake`, and `Boost`, other dependencies can be automatically installed by the root level CMake file. If you would like to manually install the dependencies (like what the author did for the original experiments), follow the below instructions.

- Base Regex Libraries
  
User can install the base regular expression matching library and adapt BLARE code on top of it. We included implementation of BLARE on top of several popular regex libraries. To run the sample experiment scripts with desired base regex library, you can install the regex libraries as followed:

- Google-RE2

    Please follow the instruction in [RE2 Github Repository](https://github.com/google/re2).

- PCRE2
  - PCRE2 library

    ```bash
    git clone --branch pcre2-10.40 --depth 1 https://github.com/PCRE2Project/pcre2
    autogen.sh
    ./configure --prefix=/usr/local             \
            --enable-unicode                    \
            --enable-jit                        \
            --enable-pcre2-16                   \
            --enable-pcre2-32                   \
            --disable-static                    &&
    make
    make install
    ```

  - jPCRE2 (a C++ wrapper of PCRE2)

    Please follow the instruction in [jPCRE2 Github Repository](https://github.com/jpcre2/jpcre2).

- Boost.Regex should have already been installed.
- ICU Regex
  
    Please download [ICU4C 72 release](https://github.com/unicode-org/icu/releases/tag/release-72-1), and follow the [installation instructions](https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-build-and-install-on-unix).

    ```bash
    tar xvfz icu4c-72_1-src.tgz
    cd icu/source
    ./configure --prefix=/usr/local/icu4c/72_1 --enable-icu-config
    make
    make install
    ```

## Instruction

We evaluate the performance of BLARE on two production workloads and one open-sourced workload. We have included the December 21st version of the open-source workload: [US Accident Dataset](https://www.kaggle.com/datasets/sobhanmoosavi/us-accidents) and the regular expressions in the repository. The newest version of the dataset has different format and may not work with the hard-coded csv parsing logic in the original codebase; consider modifiy the `read_traffic` function in the code that you will be running. To use the original version of the dataset, install `git lfs` then run

```bash
git lfs pull
```

to retrieve the dataset.

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
