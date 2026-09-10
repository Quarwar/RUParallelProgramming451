/*
    Naive approach to counting primes with multithreading. It takes roughly 64 seconds to count primes between 
    1 and 100'000'000. It uses 4 cores with 4 threads.
*/

#include <iostream>
#include <cmath>
#include <thread>
using namespace std;

//global variable (must be declared before all threads)
u_int64_t counts[4];

// threads t1-t4
void countPrime(u_int64_t start, u_int64_t n, u_int64_t batchsize, u_int64_t step, u_int64_t* output) {
    u_int64_t end, count = 0;
    while (start <= n) {
        end = min(start+batchsize-1, n);
        for (u_int64_t i = start; i <= end; i++) {
            for (u_int64_t j = 2; j <= sqrt(i); j++) {
                if (i % j == 0) {goto NOT_PRIME;}
            }
            count++;
            NOT_PRIME:;
        }
        start += step;
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
    const int n = 100'000'000; // Count primes in [1, n]
    const int batchsize = 1'000'000; // Number of values each thread handles at a time
    const int step = batchsize*4; // multiply by the number of threads (4 is for my machine)

    // start 4 threads for 4 cores
    thread t1(countPrime, 2, n, batchsize, step, &counts[0]);
    thread t2(countPrime, batchsize+2, n, batchsize, step, &counts[1]);
    thread t3(countPrime, 2*batchsize+2, n, batchsize, step, &counts[2]);
    thread t4(countPrime, 3*batchsize+2, n, batchsize, step, &counts[3]);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // add up the results from each thread
    u_int64_t total = 0;
    for (int i = 0; i < size(counts); i++) {total += counts[i];}
    cout << "Total number of primes: " << total << "\n";

    // Test runtime output (in seconds):
    std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}