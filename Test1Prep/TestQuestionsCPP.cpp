// Programming Questions

// Q1: Write a short C++ program that creates four threads. Each thread computes the sum of a different, 
// non-overlapping portion of an integer array. The main thread must obtain and print the total sum after 
// all threads finish.

#include <stdlib.h>
#include <cstdint>
#include <cmath>
#include <thread>
#include <iostream>
#include <immintrin.h>
using namespace std;

void t(uint64_t start, uint64_t batchsize, uint64_t size, int* arr, uint64_t* out) {
    uint64_t end, sum = 0;
    while (start < size) {
        end = min(start + batchsize, size);
        for (uint64_t i = start; i < end; i++) {sum += arr[i];}
        start += 4*batchsize;
    }

    *out = sum;
}

int main() {
    // input array
    uint64_t size = 1'000'000;
    int* arr = new int[size];
    for (int i = 1; i <= size; i++) {arr[i-1] = i;} // integers from 1 to 1,000,000
    
    // compute the sum
    uint64_t* sums = new uint64_t[4];
    for (int i = 0; i < 4; i++) {sums[i] = 0;}

    uint64_t batchsize = 1'000; // each thread handles a batch of 1,000 at a time
    thread t1(t, 0, batchsize, size, arr, &sums[0]);
    thread t2(t, 1000, batchsize, size, arr, &sums[1]);
    thread t3(t, 2000, batchsize, size, arr, &sums[2]);
    thread t4(t, 3000, batchsize, size, arr, &sums[3]);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // compute final sum:
    uint64_t total = 0;
    for (int i = 0; i < 4; i++) {total += sums[i];}
    cout << "Final total: " << total << "\n";

    // Check result:
    uint64_t correctTotal = (size*(size + 1)) >> 1;
    cout << "Mathematical result: " << correctTotal << "\n";

    delete[] arr;
    delete[] sums;

    return 0;
}