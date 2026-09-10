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

#include <iostream>
#include <cmath>
#include <thread>
using namespace std;

//global variable (must be declared before all threads)
u_int64_t counts[4];

// threads t1-t4
void countPrime(u_int64_t start, u_int64_t n, u_int64_t* output) {
    u_int64_t lim, count = 0;
    for (u_int64_t i = start; i <= n; i+=12) {
        lim = static_cast<u_int64_t>(sqrt(i));
        for (u_int64_t j = 5; j <= lim; j+=6) {
            if (i % j == 0 || i % (j+2) == 0) {goto NOT_PRIME;}
        }
        count++;
        NOT_PRIME:;
    }

    *output = count;
}

/*
    Prime counting algorithm using multithreading. Can be built for any number of threads.
*/
int main() {
    // Test runtime:
    std::chrono::steady_clock::time_point _start(std::chrono::steady_clock::now());

    // initialize number of primes to count and step size data
    const int n = 100'000'000; // Count primes in [1, n] (n > 3)

    // start 4 threads for 4 cores
    thread t1(countPrime, 5, n, &counts[0]);
    thread t2(countPrime, 7, n, &counts[1]);
    thread t3(countPrime, 11, n, &counts[2]);
    thread t4(countPrime, 13, n, &counts[3]);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // add up the results from each thread
    u_int64_t total = 2; // our algorithm skips 2 & 3
    for (int i = 0; i < size(counts); i++) {total += counts[i];}
    cout << "Total number of primes: " << total << "\n";

    // Test runtime output (in seconds):
    std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}