#include <stdlib.h>
#include <cstdint>
#include <cmath>
#include <thread>
#include <iostream>
#include <immintrin.h>
using namespace std;

int main() {
    // Add two arrays of eight elements (each is 32-bit)
    float arr1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float arr2[8] = {123, 14, 15, 15, -2, 3, 2.2, 23.32};

    __m256 _v1 = _mm256_loadu_ps(arr1);
    __m256 _v2 = _mm256_loadu_ps(arr2);

    __m256 v1 = _mm256_setr_ps(1, 2, 3, 4, 5, 6, 7, 8);
    __m256 v2 = _mm256_setr_ps(123, 14, 15, 15, -2, 3, 2.2, 23.32);

    __m256i v3 = _mm256_setr_epi32(1, 2, 3, 4, 5, 6, 7, 8); // integers
    //__m256i _v3 = _mm256_loadu_epi32(arr1);
    __m256d v4 = _mm256_setr_pd(1.21, 12412.1, 252.1, 212.1); // doubles

    // load arr1 and arr2
    __m256 x1 = _mm256_loadu_ps(arr1);
    __m256 x2 = _mm256_loadu_ps(arr2);
    __m256 x3 = _mm256_add_ps(x1, x2);
    // store into output array:
    float* arr = new float[8];
    _mm256_storeu_ps(arr, x3);

    // swap: 
    __m256 min = _mm256_min_ps(x1, x2);
    __m256 max = _mm256_max_ps(x1, x2);
    x1 = min;
    x2 = max;

    for (int i = 0; i < 8; i++) {
        cout << i + "# " << arr[i] << " ";
    }
    cout << "\n";

    float _z[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float _c[8] = {9, 10, 11, 12, 13, 14, 15, 16};

    __m256 z = _mm256_loadu_ps(_z);
    __m256 c = _mm256_loadu_ps(_c);
    __m256 out = _mm256_add_ps(c, _mm256_mul_ps(z, z));

    float result[8];
    _mm256_storeu_ps(result, out);

    return 0;
}