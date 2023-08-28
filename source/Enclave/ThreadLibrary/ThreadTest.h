/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
//#include <fcntl.h>
#include "string.h"
#include "mbusafecrt.h"
#include "pthread.h"
#include "sgx_thread.h"
#include "sgx_trts.h"
#include "sgx_tseal.h"
#include "rtm.h"

//#include <immintrin.h> //tsx
//#include <x86intrin.h>

/*For huge pages --> No longer available*/
/*#define FILE_NAME "/mnt/hugetlbfs/filehuge"
#define PROTECTION (PROT_READ | PROT_WRITE)*/
/* Only ia64 requires this */
/*#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_SHARED | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_SHARED)
#endif*/

#define BUFFER_SIZE 50
#define SAMPLES_SIZE 5
#define RESERVED_MEMORY 36*1024*1024
#define RESERVED_RAM 4*1024*1024
#define TEST_SET 28
#define CACHE_SET_SIZE 16
#define CACHE_SLICES 12
#define MONITORED_SETS 16 // All the possible values
#define WAYS_FILLED 16
#define NEXT_SET (1ULL << (6 + 10))
#define MON_WINDOW 16

#define ACK_TLB 0

#define TLB_L1_WAYS 4
#define TLB_L1_SETS 64
#define TLB_L2_WAYS 12
#define TLB_L2_SETS 1536
#define TLB_SIZE 4*64
#define TLB_SIZE_2 1536
#define PAGE_SIZE 4*1024
#define PAGE_BITS 12
//#define EV_SET_SIZE TLB_L1_WAYS+TLB_L2_WAYS
#define EV_SET_SIZE 4
#define RESERVED_MEMORY_TLB (EV_SET_SIZE+4)*PAGE_SIZE//*TLB_SIZE_2

#define SLICE_MASK_0 0x0002040000UL
#define SLICE_MASK_1 0x0001020000UL
#define SLICE_MASK_2 0x0000810000UL
#define SLICE_MASK_3 0x0000408000UL
#define SLICE_MASK_4 0x0000204000UL
#define SLICE_MASK_5 0x0000102000UL
#define SLICE_MASK_6 0x0000081000UL

#define PAGE_COUNT RESERVED_MEMORY/4096
#define WINDOW_SIZE 14
#define SPOILER_SETS 256 //90
#define SPOILER_PEAKS 38 //PAGE_COUNT/125

#define mfence()  __asm__ volatile("mfence;"); 
#define lfence()  __asm__ volatile("lfence;");

extern long int global_counter;    /* global counter */
extern int monitor_running;
extern int monitor_ready;
extern int sampler_ready;
//extern sgx_thread_mutex_t global_mutex;
extern pthread_mutex_t global_mutex; 
extern pthread_mutex_t mutex_start;
extern pthread_cond_t condp;
extern pthread_cond_t condSets;
extern unsigned char secret_data[BUFFER_SIZE];
extern char memory_map[RESERVED_MEMORY];
extern long int eviction_set[MONITORED_SETS*CACHE_SET_SIZE*CACHE_SLICES];
extern long int eviction_set_1[CACHE_SET_SIZE*CACHE_SLICES];
extern long int final_eviction_set[MONITORED_SETS*CACHE_SET_SIZE*CACHE_SLICES];
extern long int eviction_add[MONITORED_SETS*CACHE_SLICES];
extern long int eviction_add_1[CACHE_SLICES];
extern int ram_border;
extern uint8_t *secret_data_read;
extern long int *base_address;
extern int secret_set;
extern int fd;
extern char memory_map_tlb[RESERVED_MEMORY_TLB];
extern long int eviction_set_tlb[EV_SET_SIZE];
extern long int samples_data[MONITORED_SETS * CACHE_SET_SIZE * CACHE_SLICES * SAMPLES_SIZE];
extern long int samples_tlb[EV_SET_SIZE * SAMPLES_SIZE];
extern size_t samples_rip[MONITORED_SETS * CACHE_SET_SIZE * CACHE_SLICES * SAMPLES_SIZE];
//extern int samples_exit[MONITORED_SETS * WAYS_FILLED * CACHE_SLICES * SAMPLES_SIZE];
extern long int samples_time[MONITORED_SETS * CACHE_SET_SIZE * CACHE_SLICES * SAMPLES_SIZE];
extern long int measurements[PAGE_COUNT];
extern long int array_addresses[SPOILER_SETS][SPOILER_PEAKS];
extern long int set_addresses[SPOILER_SETS * SPOILER_PEAKS];
extern int mon_ways;

/*
 * increase_counter:
 *   Utilize thread APIs inside the enclave.
 */
long int mem_access(long int *v);
void flush_data(long int *pos_data);
void flush_desired_set(long int address_array[], int len);
long int access_timed(long int *pos_data,long int *t1);
long int access_timed_1(long int *pos_data);
long int calibrate_miss(long int *pos_data);
int probe_candidate(int number_elements, long int array_set[], long int *dir_candidate);
int probe_candidate_sorted(int number_elements, long int array_set[], long int *dir_candidate);
int probe_candidate_flush(int number_elements, long int array_set[], long int *dir_candidate);
void randomize_set(long int array[], int len);
int check_inside(long int dato, long int array_data[], int array_length);
long int hammer (long int *pos1, long int *pos2, int rounds);
long int increase_counter();
long int check_counter();
int check_running();
int check_ready();
int check_sampler();
int assign_secret (uint8_t* sealed_blob, uint32_t* data_size);
sgx_status_t unseal_data(const uint8_t *sealed_blob, size_t data_size);
void* counter (void *a);
void* fast_counter (void *);

int parity(unsigned long int v);
int addr2slice_linear(unsigned long int addr, int bitsset);
void test_tlb_l1(int ways, int sets, int rounds);
void test_tlb_l2(int ways, int sets, int rounds);
void build_tlb_eviction(int max, long int array[]);
long int prime_ev_set(unsigned long int pos_data);
long int measure_access(long int *pos_data);
void address_aliasing(int w, int x, int ini, long int meas[PAGE_COUNT]);
int peak_search (long int simple_set[CACHE_SET_SIZE*CACHE_SLICES],long int meas[PAGE_COUNT]);


