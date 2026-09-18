/*

Parallel Programming HW01
Author: Joseph Signorile
Cite: ChatGPT (mostly syntax + method name lookup; code design is entirely done by me)

1. Run the code single-threaded
2. Benchmark the code multi-threaded writing to *pcount every time (Slower)
3. Benchmark the code multi-threaded with a pool of 2 threads (4 optional)
4. Now you write partitioning. Divide not into 2 but into chunks.

Table of Results: n = 100'000'000 (5,761,455 primes)
Single-threaded: 78.63 seconds
Naive memory-write multi-threaded: 31.52 seconds
No memory-write in loop multi-threaded: 31.14 seconds // about the same
Correctly partitioned multi-threaded: 27.14 seconds

Mathematically optimized solution:
Prime Number Wheel Multi-threaded: 8.98 seconds

*/

#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
using namespace std;

//global variable (stores the output for all solutions below - 4 slots for 4 threads)
uint64_t counts[4];

// Single-threaded solution (simplest, most naive approach):
void countPrimeSingleThreaded(uint64_t start, uint64_t n, uint64_t* output) {
    uint64_t lim, count = 0;
    if (start == 1) {count--;}
    for (uint64_t i = start; i <= n; i++) {
        lim = static_cast<uint64_t>(sqrt(i));
        for (uint64_t j = 2; j <= lim; j+=1) {
            if (i % j == 0) {goto NOT_PRIME;}
        }
        count++;
        NOT_PRIME:;
    }

    *output = count;
}

// Naive multi-threaded solution: Writes to memory every loop, badly partitions into 4 continguous chunks:
void countPrimeNaiveMultiThreaded(uint64_t start, uint64_t end, uint64_t* output) {
    uint64_t lim;
    if (start == 1) {(*output)--;}
    for (uint64_t i = start; i <= end; i++) {
        lim = static_cast<uint64_t>(sqrt(i));
        for (uint64_t j = 2; j <= lim; j+=1) {
            if (i % j == 0) {goto NOT_PRIME;}
        }
        (*output)++;
        NOT_PRIME:;
    }
}

// Slightly better multi-threaded solution: badly partitions into 4 continguous chunks, but does not write to memory every loop:
void countPrimeMultiThreadedImproved(uint64_t start, uint64_t end, uint64_t* output) {
    uint64_t lim, count = 0;
    if (start == 1) {count--;}
    for (uint64_t i = start; i <= end; i++) {
        lim = static_cast<uint64_t>(sqrt(i));
        for (uint64_t j = 2; j <= lim; j+=1) {
            if (i % j == 0) {goto NOT_PRIME;}
        }
        count++;
        NOT_PRIME:;
    }

    *output = count;
}

// Better muluti-threaded solution with even partitioning:
// Thread t1: [1, batchsize] -> [1 + step, batchsize + step] -> [1 + 2*step, batchsize + 2*step]
// Thread t2: [batchsize + 1, 2*batchsize]
// Thread t3: [2*batchsize + 1, 3*batchsize]
// Thread t4: [3*batchsize + 1, 4*batchsize]
void countPrimeMultiThreadedPartitioned(uint64_t start, uint64_t n, uint64_t batchsize, uint64_t step, uint64_t* output) {
    uint64_t lim, bound, count = 0;
    if (start == 1) {count--;}
    for (uint64_t k = start; k <= n; k += step) {
        bound = min(n, k + batchsize - 1);
        for (uint64_t i = k; i <= bound; i++) {
            lim = static_cast<uint64_t>(sqrt(i));
            for (uint64_t j = 2; j <= lim; j+=1) {
                if (i % j == 0) {goto NOT_PRIME;}
            }
            count++;
            NOT_PRIME:;
        }
    }
    *output = count;
}

/*
    Prime Number Wheel for 4 cores. Every prime greater than 3 is of the form 6k +- 1; therefore, each of the four threads can
    handle four seperate partitions of these numbers:
    T1: 6*(2k-1) - 1
    T2: 6(2k-1) + 1
    T3: 6*(2k) - 1
    T4: 6*(2k) + 1

    All the above threads increment by 12 for each increment to k, covering all possible primes (with the exception of 2 & 3).
    The index k starts at 1 and keeps increasing until the resulting value exceeds the total number of integers we want to check up to. 

    So far, the code can compute the correct answer for primes up to 100 million in roughly 10 seconds.
*/
void countPrime(uint64_t start, uint64_t n, uint64_t* output) {
    uint64_t lim, count = 0;
    for (uint64_t i = start; i <= n; i+=12) {
        lim = static_cast<uint64_t>(sqrt(i));
        for (uint64_t j = 5; j <= lim; j+=6) {
            if (i % j == 0 || i % (j+2) == 0) {goto NOT_PRIME;}
        }
        count++;
        NOT_PRIME:;
    }

    *output = count;
}

/*
    Prime counting algorithms using multithreading. Can be built for any number of threads.
*/
int main() {
    // Test runtime:
    chrono::steady_clock::time_point _start(chrono::steady_clock::now());

    // initialize number of primes to count and step size data
    const int n = 100'000'000; // Count primes in [1, n] (n > 3)
    const int batchsize = 1'000;
    const int step = 4*batchsize; // for 4 available threads

    // naive single-threaded solution:
    // countPrimeSingleThreaded(1, n, &counts[0]);
    // cout << "Total number of primes: " << counts[0] << "\n";

    // Multi-threaded solutions:
    // ====================================================================================

    // naive multi-threaded solution (writes every loop, requires n to be divisible by 4):
    // thread t1(countPrimeNaiveMultiThreaded, 1, n/4, &counts[0]);
    // thread t2(countPrimeNaiveMultiThreaded, n/4 + 1, n/2, &counts[1]);
    // thread t3(countPrimeNaiveMultiThreaded, n/2 + 1, 3*(n/4), &counts[2]);
    // thread t4(countPrimeNaiveMultiThreaded, 3*(n/4) + 1, n, &counts[3]);

    // improved multi-threaded solution (does not write every loop, still a bad partition):
    // thread t1(countPrimeMultiThreadedImproved, 1, n/4, &counts[0]);
    // thread t2(countPrimeMultiThreadedImproved, n/4 + 1, n/2, &counts[1]);
    // thread t3(countPrimeMultiThreadedImproved, n/2 + 1, 3*(n/4), &counts[2]);
    // thread t4(countPrimeMultiThreadedImproved, 3*(n/4) + 1, n, &counts[3]);

    // correctly partitioned multi-threaded solution:
    // thread t1(countPrimeMultiThreadedPartitioned, 1, n, batchsize, step, &counts[0]);
    // thread t2(countPrimeMultiThreadedPartitioned, batchsize + 1, n, batchsize, step, &counts[1]);
    // thread t3(countPrimeMultiThreadedPartitioned, 2*batchsize + 1, n, batchsize, step, &counts[2]);
    // thread t4(countPrimeMultiThreadedPartitioned, 3*batchsize + 1, n, batchsize, step, &counts[3]);

    // t1.join();
    // t2.join();
    // t3.join();
    // t4.join();

    // // add up the results from each thread
    // uint64_t total = 0; 
    // for (int i = 0; i < size(counts); i++) {total += counts[i];}
    // cout << "Total number of primes: " << total << "\n";

    // ====================================================================================

    // Prime Number Wheel:
    // ====================================================================================

    thread t1(countPrime, 5, n, &counts[0]);
    thread t2(countPrime, 7, n, &counts[1]);
    thread t3(countPrime, 11, n, &counts[2]);
    thread t4(countPrime, 13, n, &counts[3]);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // add up the results from each thread
    uint64_t total = 2; // our algorithm skips 2 & 3
    for (int i = 0; i < size(counts); i++) {total += counts[i];}
    cout << "Total number of primes: " << total << "\n";

    // ====================================================================================

    // Test runtime output (in seconds):
    chrono::steady_clock::time_point _end(chrono::steady_clock::now());
    cout << chrono::duration_cast<chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}

