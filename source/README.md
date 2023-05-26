# Clonebuster

This code was tested on Ubuntu 18.04 in different CPUs
It requires SGX to be enabled on the BIOS of a capable machine and the Intel SDK to be installed.

The folders are organized as follows

* App contains the untrusted app that executes the ecalls
* Enclave contains the code where all the relevant functions to explore the DRAM, create the eviction sets and monitor the sets are coded
* Include 


#### Architecture details

We note that in order to work properly, the hardware details referring to each machine need to be changed in 

Enclave/ThreadLibrary/cache_details.h

/*Cache and memory architecture details*/
#define CACHE_SIZE 12 //MB
#define CPU_CORES 6
#define CACHE_SET_SIZE 16 //Ways
#define CACHE_SLICES 12
#define SETS_PER_SLICE 1024
#define BITS_SET 10
#define BITS_LINE 6 //64 bytes per cache line

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

