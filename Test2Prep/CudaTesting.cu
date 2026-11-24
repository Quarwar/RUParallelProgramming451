/*
    Cuda Testing:
    Compile: nvcc CudaTesting.cu -o CudaTesting
    Run on Colab with NVIDIA GPU (T4)
*/

#include <cstdint>
#include <iostream>
#include <cuda_runtime.h>
using namespace std;

// kernel (__global__ specifies a method callable from the CPU, executed on the GPU)
__global__ void addMatrices(const uint32_t rows, const uint32_t cols, const float *A, const float *B, float *C) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index < rows*cols) {
        C[index] = A[index] + B[index];
    }
}

// atomic operations:
__global__ void testing(int* counter) {
    atomicAdd(counter, 1); // adds one to counter (equivalent of #pragma omp critical)
}

int main() {
    // Create variables on CPU
    uint32_t rows = 100, cols = 100;
    uint32_t size = rows * cols * sizeof(float);
    float *A = new float[rows*cols];
    float *B = new float[rows*cols];
    float *C = new float[rows*cols];

    // Initialize with dummy data:
    for (uint32_t i = 0; i < rows*cols; i++) {
        A[i] = i;
        B[i] = 2*i;
        C[i] = 0;
    }

    // Copy variables to GPU
    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size); // takes in address of a pointer: d_A now stores GPU memory address
    // cout << d_A[0]; // illegal; d_A is a pointer containing a GPU address, CPU cannot dereference 
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size); // we allocate memory for C, no need to copy the values over to C
    cudaMemcpy(d_A, A, size, cudaMemcpyHostToDevice); // destination (ptr), source (ptr), size (num bytes), type
    cudaMemcpy(d_B, B, size, cudaMemcpyHostToDevice);
    // cudaMemcpy(d_C, C, size, cudaMemcpyHostToDevice); // unnecessary: C is only written to

    uint32_t threads = 256; // using 256 threads per block is a standard approach
    uint32_t blocks = (rows * cols + (threads - 1))/threads; // number of blocks needed for rows*cols threads
    // performs computations on the copied GPU memory (through d_C)
    addMatrices<<<blocks, threads>>>(rows, cols, d_A, d_B, d_C); // launches kernel (running on GPU), program host continues running

    // waits for kernel to finish
    // cudaDeviceSynchronize(); // synchronizes the host with any methods running on device (GPU)

    // Copy values back to CPU
    // synchronized method; even though the program keeps running, this waits for the kernel
    cudaMemcpy(C, d_C, size, cudaMemcpyDeviceToHost);
    // Free allocated memory on GPU
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    delete[] A;
    delete[] B;
    delete[] C;
}