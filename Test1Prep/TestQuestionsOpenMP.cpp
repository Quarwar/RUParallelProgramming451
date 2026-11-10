#include <iostream>
#include <omp.h>
#include <cstdint>
using namespace std;

void sum1() {
    int sum = 0;
    int size = 4;
    int maxThreads = omp_get_max_threads();
    int* arr = new int[size];
    for (int i = 0; i < size; i++) {arr[i] = 0;}

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i <= 1000; i++) {
        arr[omp_get_thread_num()] += i;
    }

    for (int i = 0; i < size; i++) {sum += arr[i];}

    cout << "Sum: " << sum << '\n';
    uint64_t correctSum = (1000*(1000+1)) >> 1;
    cout << "Correct Sum: " << correctSum << '\n';

    delete[] arr;
}

void sum2() {
    int sum = 0; 
    #pragma omp parallel for reduction(+:sum) // gives each thread a private copy of sum, then adds each copy at the end
    for (int i = 0; i <= 1000; i++) {
        sum += i;
    }

    cout << "Sum: " << sum << "\n";
}

void exercise1() {
    const int N = 100000;
    int* arr = new int[N];
    for (int i = 0; i < N; i++) {arr[i] = i;}

    long long sum = 0;
    int maximum = 0;
    int evenCount = 0;

    // TODO: Add your OpenMP directive here.
    // Use 4 threads and schedule(static, 100).
    // Use reductions for all three statistics.
    #pragma omp parallel for num_threads(4) schedule(static, 100) reduction(+:sum, evenCount) reduction(max:maximum)
    for (int i = 0; i < N; i++) {
        sum += arr[i];

        if (arr[i] > maximum)
            maximum = arr[i];

        if (arr[i] % 2 == 0)
            evenCount++;
    }

    cout << "Sum: " << sum << '\n';
    cout << "Maximum: " << maximum << '\n';
    cout << "Even count: " << evenCount << '\n';

    delete[] arr;
}

int exercise2() {
    int sharedValue = 0;
    int sum = 0;

    // TODO: Create a parallel region with 4 threads.
    #pragma omp parallel num_threads(4)
    {
        int id = omp_get_thread_num();

        // TODO: Print the thread ID.
        cout << "Thread ID: " << id << "\n";

        // TODO: Initialize sharedValue to 100 using single.
        #pragma omp single
        sharedValue = 100;

        // TODO: Add a barrier so all threads wait for initialization.
        #pragma omp barrier

        // TODO: Safely add id to sum using critical.
        #pragma omp critical
        sum += id;

        cout << "Thread " << id
             << " sees sharedValue = " << sharedValue << '\n';
    }

    cout << "Final sum: " << sum << '\n';
    return 0;
}

int main() {
    // sum1();
    // sum2();
    // exercise1();
    exercise2();
    return 0;
}