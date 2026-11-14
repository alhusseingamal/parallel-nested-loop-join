#pragma once
#include <cuda.h>
#include "gpu_utils.cuh"
__global__ void NLJ_kernel(int* A, int nA, int* B, const int nB, int* resultIndices, int* resultIndicesCount, int num_conditions, CompareOp* condition_ops);