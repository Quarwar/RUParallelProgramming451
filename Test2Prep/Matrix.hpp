#pragma once
#include <cstdint>

class Matrix {
    private:
    float* m; // row-major order
    uint32_t rows, cols;

    public:
    // Constructor
    Matrix(uint32_t rows, uint32_t cols) : rows(rows), cols(cols), m(new float[rows*cols]) {
        for (uint32_t i = 0; i < rows*cols; i++) {m[i] = 0.0;}
    } 
    // Destructor
    ~Matrix() {
        delete []m;
    }
};