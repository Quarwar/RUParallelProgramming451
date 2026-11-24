#include <cuda_runtime.h>
#include <cstdint>
#include <iostream>
#include <iomanip>

// NxN matrix multiply kernel (runs on GPU)
__global__ void matrixMultiply(uint32_t N, const float *A, float *B, float *C) {
    uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t j = blockIdx.y * blockDim.y + threadIdx.y;
    /*  For one-dimensional blocks: 
        blockDim : number of threads per block
        blockIdx.x : block ID : [0, (numBlocks-1)] 
        threadIdx.x : thread ID : [0, (numThreadsInBlock-1)]
    */
    
    // INCORRECT CODE: synchronization is correct, but cannot launch another multi-threaded function from a kernel
    // // ensures that only one thread runs this code (similar to #pragma omp single)
    // if (i == 0 && j == 0) {
    //     uint32_t threads = N;
    //     uint32_t blocks = (N + 1)/2;
    //     matrixTranspose<<<blocks, threads>>>(N, B);
    // }
    // __syncthreads(); // Also incorrect: only synchronizes within a block

    // perform the matrix multiply
    if (i < N && j < N) {
        float sum = 0;
        for (uint32_t k = 0; k < N; k++) {
            sum += A[N * i + k]*B[N * j + k];
        }
        C[N * i + j] = sum;
    }
}

// For NxN matrix: N(N-1)/2 swaps
// Assumes number of blocks is N, with N threads per block
__global__ void matrixTranspose(uint32_t N, float *A) {
    uint32_t i = threadIdx.x;
    uint32_t j = blockIdx.x;

    if (i < N - j - 1) {
        uint32_t row = j;
        uint32_t col = i + j + 1;

        float temp = A[N * row + col];
        A[N * row + col] = A[N * col + row];
        A[N * col + row] = temp;
    }
}

void printMatrix(const float *A, uint32_t N) {
    for (uint32_t i = 0; i < N; i++) {
        for (uint32_t j = 0; j < N; j++) {
            std::cout << std::setw(8) << A[N * i + j] << " ";
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

int main() {
    // Create variables on CPU
    uint32_t N = 100;
    uint32_t size = N * N * sizeof(float);
    float *A = new float[N * N];
    float *B = new float[N * N];
    float *C = new float[N * N];

    // Initialize with dummy data:
    for (uint32_t i = 0; i < N * N; i++) {
        A[i] = i;
        B[i] = 2*i;
        C[i] = 0;
    }

    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size);
    cudaMemcpy(d_A, A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, size, cudaMemcpyHostToDevice);

    dim3 threads(16, 16);
    dim3 blocks((N + 15)/16, (N + 15)/16);
    matrixTranspose<<<N-1, N>>>(N, d_B);
    matrixMultiply<<<blocks, threads>>>(N, d_A, d_B, d_C);

    cudaMemcpy(C, d_C, size, cudaMemcpyDeviceToHost);
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    printMatrix(A, N);
    printMatrix(B, N);
    printMatrix(C, N);

    delete[] A;
    delete[] B;
    delete[] C;
}