# Bayesian Calibration of Double-sided Silicon Strip Detectors

This project implements an energy calibration method for double-sided silicon strip detectors, as described in [M. Reese et. al.](https://doi.org/10.1016/j.nima.2015.01.032). The method employs Bayesian inference to calculate the set of intrinsic calibration coefficients of a given silicon detector. The workflow involves data extraction, data filtering, slope calculation, calibration coefficient computation, and visualization. Designed specifically for the Super Enge Split-pole Spectrograph and Silicon Array for Branching Ratio Experiments (SESPS + SABRE) experimental setup at Florida State University's John D. Fox accelerator laboratory, it necessitates the sorted and analyzed output files from the [SPS-SABRE EventBuilder](https://github.com/sesps/SPS_SABRE_EventBuilder).

## Dependencies

This program is built with:

+ **GNU Make 4.3** for managing the build process.
+ **C++ compiler with C++17 support**  (GCC 7.0 and above, or equivalent) for compiling the source code.
+ **ROOT (version 6.26/10)** for handling data analysis and visualization.
+ **OpenMP**  for enabling parallel processing.

## Installation

First, clone the GitHub repository:

`git clone https://github.com/KhangPham0/SABRE_gain_matcher.git`

Navigate to the project directory:

`cd SABRE_gain_matcher`

Compile the program and generate the shared library with the provided makefile:

`make`

This command will also create all necessary directories, compile all source files, generate ROOT dictionaries, and build a shared library.

## Usage

Once the program is successfully built, run it using the following command:

`./bin/gainMatch <run_number> <config_file>`

Where:

+ `<run_number>` is an integer representing the run number of the input file you want to use for to do gain match.
+ `<config_file>` is the name of a configuration file. Example configuration files named `det#_config.txt` (where # can go from 0 to 4) are already provided in the repo.

For example, to run the program with run number 83 and configuration file `det0_config.txt`, use:

`./bin/gainMatch 83 det0_config.txt`

The program will load the required shared library, process the input arguments and the configuration file, extract and filter necessary data from the specified source, calculate slopes of the filtered data using Bayesian inference, calculate calibration coefficients using chi-square minimization, and generate and save plots of the results in the `outputs` directory.

Please note that the example configuration files assume that the "eventbuild" root files are located inside the `inputs` directory and follow the naming format `run_#.root` where # is an integer. If you create your own configuration files, they should specify details such as the input directory, detector ID, channel map file, and other relevant parameters.

## Cleaning up

To clean up the generated files and start fresh, run:

`make clean`

This command will remove the compiled binaries, object files, shared library, and the ROOT dictionaries generated during the build process.

## Acknowledgements

+ This implementaion is based on the method proposed in [Automatic intrinsic calibration of double-sided silicon strip detectors](https://doi.org/10.1016/j.nima.2015.01.032)
+ Elements such as the `ChannelMap` class, data structures, and ROOT dictionary definitions in this project are borrowed from the [SPS-SABRE EventBuilder](https://github.com/sesps/SPS_SABRE_EventBuilder)
+ The majority the algorithm in this project was initially developed by Dr. Sudarsan Balakrishnan. His implementation of the mentioned paper can be found [here](https://github.com/sudb92/a.dig).