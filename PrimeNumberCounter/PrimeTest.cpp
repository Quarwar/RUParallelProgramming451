#include <iostream>
#include <cmath>
#include <chrono>
using namespace std;

u_int64_t countprimes(u_int64_t a, u_int64_t b) {
    u_int64_t count = 0;
    for (u_int64_t i = a; i <= b; i++) {
        for (u_int64_t j = 2; j <= sqrt(i); j++) {
            if (i % j == 0) {goto NOT_PRIME;}
        }
        count++;
        NOT_PRIME:;
    }

    if (a == 1) {count--;}
    return count;
}

int main() {
    // Test runtime:
    std::chrono::steady_clock::time_point _start(std::chrono::steady_clock::now());
    cout << countprimes(1,1000) << '\n';
    // Test runtime output (in seconds):
    std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(_end - _start).count() << " seconds\n";
    return 0;
}