# Clonebuster

This is the artifact submission corresponding to the conditionally accepted paper #207. In our paper, we introduced a clone-detection tool for Intel SGX, dubbed CloneBuster, that uses a cache-based covert channel for enclaves to detect clones (i.e., other enclaves loaded with the same binary). 

Our artifact consists of the source code of CloneBuser with the instructions to compile the code, run it, and install the software dependencies. It requires a machine that supports SGX 1.0 and the Intel SDK installed. Our artifact has been tested on Ubuntu 18.04 and 20.04, with the SDK version 2.18 (although it should work with other versions of the SDK). Once the code is compiled, the application launches the enclave and provides the enclave with details about the hardware (cache size an associativity) and number of monitored ways of the cache as input. Then, the enclave creates all the eviction sets and the covert channel and monitors it when triggered. As output, the enclave writes a file with the cache samples (reading times of the data in the eviction sets). Data obtained from these files has been used in Section 6 of paper #207 to assess the performance of CloneBuster.

## Organization

`Warning: this is a proof-of-concept, it is mainly useful for collecting data and evaluating it as done in the CloneBuste paper.
Use it under your own risk.`

This code was tested on Ubuntu 18.04 and 20.04 in different CPU models.
It requires SGX to be enabled on the BIOS of a capable machine and the Intel SDK to be installed. Further details on how to do that
are included at the end of this readme so the interested user can install them from scratch. 

The folders are organized as follows

* App contains the untrusted app that comunicates with the enclave
* Enclave contains the code where all the relevant functions to create the eviction sets and monitor the covert channel are coded
* Include is not relevant for this example

It also includes a Makefile that compiles the enclave and the app.

#### Architecture details

We note that in order to work properly, the hardware details referring to each machine need to be changed in 

Enclave/ThreadLibrary/cache_details.h

```

/*Cache and memory architecture details*/
#define CACHE_SIZE 12 //MB
#define CPU_CORES 6
#define CACHE_SET_SIZE 16 //Ways
#define CACHE_SLICES 12
#define SETS_PER_SLICE 1024
#define BITS_SET 10
#define BITS_LINE 6 //64 bytes per cache line

```

Further there are other parameters that have to be adjusted depending on the experiment that one needs to execute, 
those are all given as #defines, and are split among different files including ThreadTest.h, ThreadTest.cpp and Enclave.cpp

# How to run it

The folder includes a Makefile that will get the code compiled and generate an enclave and the executable app.

In this version of the code app should be called with a file_name as an argument. All the measurements and data 
will be then written to that file. The amount of data that is written for each of the experiments is hardcoded in a #define

Once the app is running it will permanently run (while (1)) and request for user input M that refers to the number of ways to be monitored

Since this version does not synchronize the execution of the 2 clones, if the results for the detection need 
to be obtained, one should launch one instance in normal mode, wait until it has executed the initialisation 
phase and then compile a second one in Attack mode (change the Define). The one in attack mode will run permanently and has 
to be killed manually


## Test

./app output_file.txt

