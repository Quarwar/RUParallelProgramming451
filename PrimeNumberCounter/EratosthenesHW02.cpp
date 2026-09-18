/*

Parallel Programming HW02
Author: Joseph Signorile
Cite: Reference code from https://github.com/RU-ECE/ECE451-Parallel/blob/main/sessions/03/02_bitwise_eratosthenes.cpp, 
ChatGPT for syntax and debugging help (the code itself was mostly written by me)    
HW02: Prime numbers using Eratosthenes mega uber bit tricks

Runtime: .131 seconds

*/

#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
using namespace std;

//global variable (stores the output for all solutions below - 4 slots for 4 threads)
uint64_t counts[5] = {0, 0, 0, 0, 0};

// global bit array data:
uint64_t n = 100'000'000; // total number of values to check
uint64_t num_words = n/128 + 1;
uint64_t* p = new uint64_t[num_words];; // pointer to bit array

// let's store primes as 0 and not prime (composite) as 1
// to set a bit we shift the mask to the right spot and OR it in
void clear_prime(uint64_t i) {
	// this is storing only odd numbers in each mask!
	// note 1LL is crucial. If you write just 1 it's an int.
	// (1 << 40) would be 0   (1LL << 40) is 10000000000000000000000000....
	p[i/128] |= (1LL << ((i%128) >> 1));
}

bool is_prime(uint64_t i) {
	return (p[i / 128] & (1LL << ((i%128) >> 1))) == 0;
}

void eratosthenesOfSqrt(uint64_t* output) {
    uint64_t limit = static_cast<uint64_t>(sqrt(n)), count = 1;
    clear_prime(1);

    for (uint64_t i = 3; i <= limit; i += 2) {
        if (is_prime(i)) {
            count++;
            for (uint64_t j = i * i; j <= limit; j += 2 * i) {clear_prime(j);}
        }
    }

    *output = count;
}

void eratosthenesMultithreaded(uint64_t start, uint64_t end, uint64_t* output) {
    uint64_t limit = static_cast<uint64_t>(sqrt(n));

    for (uint64_t prime = 3; prime <= limit; prime += 2) {
        if (!is_prime(prime)) {continue;}

        uint64_t first = ((start + prime - 1) / prime) * prime;
        if (first % 2 == 0) {first += prime;}
        first = max(first, prime * prime);

        for (uint64_t j = first; j <= end; j += 2 * prime) {
            clear_prime(j);
        }
    }
    
    // count the final total for this thread
    uint64_t count = 0;
    if (start % 2 == 0) {start++;}
    for (uint64_t i = start; i <= end; i += 2) {
        if (is_prime(i)) {count++;}
    }

    *output = count;
}

/*
    Prime counting using bitwise Eratosthenes.
*/
int main() {
    // Test runtime:
    chrono::steady_clock::time_point _start(chrono::steady_clock::now());

    // Initialize data:
    for (int i = 0; i < num_words; i++) {p[i] = 0;}

    // calculate primes below sqrt(n):
    eratosthenesOfSqrt(&counts[4]);

    const int start = sqrt(n) + 1;
    const int batchsize = (n - start)/4; // In this case, we don't have any issues since n-start does divide by 4

    // multi-thread the rest:
    thread t1(eratosthenesMultithreaded, start, start+batchsize-1, &counts[0]);
    thread t2(eratosthenesMultithreaded, start+batchsize, start+2*batchsize-1, &counts[1]);
    thread t3(eratosthenesMultithreaded, start+2*batchsize, start+3*batchsize-1, &counts[2]);
    thread t4(eratosthenesMultithreaded, start+3*batchsize, start+4*batchsize, &counts[3]);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    uint64_t total = 0; 
    for (int i = 0; i < size(counts); i++) {total += counts[i];}
    cout << "Total number of primes: " << total << "\n";

    // Test runtime output (in seconds):
    chrono::steady_clock::time_point _end(chrono::steady_clock::now());
    cout << chrono::duration_cast<chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}