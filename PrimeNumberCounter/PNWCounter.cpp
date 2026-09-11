/*
    Prime Number Wheel for 4 cores. Uses the prime number wheel for 2, 3, and 5:
    The 30-coprime sequence: 1, 7, 11, 13, 17, 19, 23, 29
*/

#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
using namespace std;

//global variable (must be declared before all threads)
uint64_t counts[4];

// threads t1-t4
void countPrime(uint64_t start, vector<uint64_t> inc, uint64_t n, uint64_t* output) {
    uint64_t lim, count = 0, i = start;
    while (true) {
        for (uint64_t k : inc) {
            uint64_t l = i+k;
            if (l > n) {goto END;}
            lim = static_cast<uint64_t>(sqrt(l));
            for (uint64_t j = 7; j <= lim; j+=6) {
                if (l % j == 0 || l % (j+4) == 0) {goto NOT_PRIME;}
            }
            count++;
            NOT_PRIME:;
        }

        i+=30;
    }
    END:;

    *output = count;
}

/*
    Prime counting algorithm using multithreading. Can be built for any number of threads.
*/
int main() {
    // Test runtime:
    std::chrono::steady_clock::time_point _start(std::chrono::steady_clock::now());

    // initialize number of primes to count and step size data
    const int n = 100'000'000; // Count primes in [1, n] (n > 30)

    // start 4 threads for 4 cores
    thread t1(countPrime, 41, vector<uint64_t>{0, 2}, n, &counts[0]);
    thread t2(countPrime, 31, vector<uint64_t>{0, 6}, n, &counts[1]);
    thread t3(countPrime, 47, vector<uint64_t>{0, 2}, n, &counts[2]);
    thread t4(countPrime, 53, vector<uint64_t>{0, 6}, n, &counts[3]);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // add up the results from each thread
    uint64_t total = 10; // we start with the primes between 1 and 30
    for (int i = 0; i < size(counts); i++) {total += counts[i];}
    cout << "Total number of primes: " << total << "\n";

    // Test runtime output (in seconds):
    std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}