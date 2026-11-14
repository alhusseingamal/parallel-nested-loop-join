#include <iostream>
#include <cuda.h>
#include <math.h>
#include <chrono>
#include "gpu_utils.cuh"

#define N 21000 // Number of rows in table R
#define M 50000 // Number of rows in table S
#define MAX_SIZE (N * M)
#define TILE_SIZE 1024
#define MAX_NUM_CONDITIONS 2
#define MAX_NUM_A_COLS 2
#define MAX_NUM_B_COLS 2

__global__ void NLJ_kernel(int* A, int nA, int* B, const int nB, int* resultIndices, int* resultIndicesCount, int num_conditions, CompareOp* condition_ops) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    // load row i into registers
    int reg_A[MAX_NUM_A_COLS];
    for (int c = 0; c < num_conditions; ++c) {
        if (i < nA) {
            reg_A[c] = A[nA * c + i]; // coalesced reads
        }
    }
    __shared__ int sh_B[MAX_NUM_B_COLS][TILE_SIZE];
    for (unsigned int stride = 0; stride < nB; stride += TILE_SIZE) 
    {
        // load a tile of B into shared memory
        unsigned int load_pos = stride + threadIdx.x;
        if (load_pos < nB) {
            for (int c = 0; c < num_conditions; c++) {
                sh_B[c][threadIdx.x] = B[nB * c + load_pos];
            }
        }
        __syncthreads();

        if (i < nA) {
            unsigned int tile_end = min(stride + TILE_SIZE, nB);
            for (int j = 0; j < (tile_end - stride); ++j) {
                bool match = true;
                // Check all join conditions
                for (int c = 0; c < num_conditions; c++) {
                    int valA = reg_A[c];
                    int valB = sh_B[c][j];
                    CompareOp op = condition_ops[c];

                    if (!compare_values(valA, valB, op)) {
                        match = false;
                        break;
                    }
                }
                // –– new, warp-aggregated output ––
                unsigned int ballot = __ballot_sync(0xffffffff, match);       // 32-bit mask of matches
                int           nm    = __popc(ballot);                         // #matches in this warp
                if (nm) {
                    unsigned int base = warp_reserve(nm, resultIndicesCount); // one atomic per *warp*
                    // offset of *my* result inside this warp
                    unsigned int offset = __popc(ballot & lane_mask_lt());
                    if (match && base + offset < MAX_SIZE) {
                        int out = (base + offset) << 1;   // * 2
                        resultIndices[out]     = i;
                        resultIndices[out + 1] = stride + j;
                    }
                }
            }
        }
    }
}

Table nested_loop_join_gpu(const Table& table_R, const Table& table_S, 
                          const std::vector<JoinCondition>& conditions) {
    int num_rows_R = table_R.rows.size();
    int num_rows_S = table_S.rows.size();
    Table result;
    
    // ----------------- PRE-PROCESSING ----------------------
    auto start = std::chrono::high_resolution_clock::now();

    // Prepare result column names
    result.column_names = table_R.column_names;
    for (const auto& col : table_S.column_names) {
        if (std::find(table_R.column_names.begin(), table_R.column_names.end(), col) == table_R.column_names.end()) {
            result.column_names.push_back(col);
        }
    }

    // Retrieve the Query Conditions
    CompareOp* h_condition_ops = new CompareOp[conditions.size()];
    for (size_t i = 0; i < conditions.size(); ++i) { h_condition_ops[i] = conditions[i].op; }

    // Pre-compute column indices for conditions (i.e. the columns to be compared)
    std::vector<std::pair<int, int>> condition_indices;
    for (const auto& cond : conditions) {
        int col_idx_R = table_R.get_column_index(cond.column_R);
        int col_idx_S = table_S.get_column_index(cond.column_S);
        if (col_idx_R == -1 || col_idx_S == -1) {
            std::cerr << "Join column not found in one of the tables: " << cond.column_R << " or " << cond.column_S << "\n";
            return result;
        }
        condition_indices.emplace_back(col_idx_R, col_idx_S);
    }
    int num_conditions = condition_indices.size();

    // Allocate and copy conditional columns for R and S
    int* h_R_cols = new int[num_rows_R * num_conditions];
    int* h_S_cols = new int[num_rows_S * num_conditions];
    for (int c = 0; c < num_conditions; c++) {
        // Copy column data from table_R to host array
        int col_idx = condition_indices[c].first;
        copy_column_to_array(table_R, col_idx, h_R_cols + c * num_rows_R);
        // Copy column data from table_S to host array
        col_idx = condition_indices[c].second;
        copy_column_to_array(table_S, col_idx, h_S_cols + c * num_rows_S);
    }
    
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = stop - start;
    std::cout << "Data Pre-processing time: " << elapsed.count() << " seconds\n";
    // -------------------------------------------------------
    start = std::chrono::high_resolution_clock::now();
    
    // Device Memory Allocation
    int *d_R_cols, *d_S_cols, *d_result_indices, *d_result_count;
    CompareOp* d_condition_ops;
    cudaMalloc(&d_R_cols, num_rows_R * num_conditions * sizeof(int));
    cudaMalloc(&d_S_cols, num_rows_S * num_conditions * sizeof(int));
    cudaMalloc(&d_result_indices, 2000 * 2 * sizeof(int));
    cudaMalloc(&d_result_count, sizeof(int));
    cudaMalloc(&d_condition_ops, num_conditions * sizeof(CompareOp));
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "Device memory allocation time: " << elapsed.count() << " seconds\n";
    // -------------------------------------------------------
    
    // Copy data to device
    start = std::chrono::high_resolution_clock::now();
    cudaMemcpy(d_R_cols, h_R_cols, num_rows_R * num_conditions * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_S_cols, h_S_cols, num_rows_S * num_conditions * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_condition_ops, h_condition_ops, num_conditions * sizeof(CompareOp), cudaMemcpyHostToDevice);
    cudaMemset(d_result_count, 0, sizeof(int));
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "Data copy to device time: " << elapsed.count() << " seconds\n";
    // -------------------------------------------------------


    // Launch kernel
    dim3 ThreadsPerBlock(TILE_SIZE, 1, 1);
    dim3 BlocksPerGrid((num_rows_R + ThreadsPerBlock.x - 1) / ThreadsPerBlock.x);
    start = std::chrono::high_resolution_clock::now();
    NLJ_kernel<<<BlocksPerGrid, ThreadsPerBlock>>>(d_R_cols, num_rows_R, d_S_cols, num_rows_S, d_result_indices, d_result_count, num_conditions, d_condition_ops);
    cudaDeviceSynchronize();
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "GPU Kernel execution time: " << elapsed.count() << " seconds\n";

    // Check for errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) << std::endl;
    }

    // Copy results back
    start = std::chrono::high_resolution_clock::now();
    int h_result_count;
    cudaMemcpy(&h_result_count, d_result_count, sizeof(int), cudaMemcpyDeviceToHost);
    
    int* h_result_indices = new int[h_result_count * 2];
    cudaMemcpy(h_result_indices, d_result_indices, h_result_count * 2 * sizeof(int), cudaMemcpyDeviceToHost);
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "Result copy back time: " << elapsed.count() << " seconds\n";

    // Build result table
    start = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < h_result_count; k++) {
        int i = h_result_indices[k * 2];
        int j = h_result_indices[k * 2 + 1];
        
        Row row = table_R.rows[i];
        for (size_t col_idx = 0; col_idx < table_S.rows[j].size(); ++col_idx) {
            const auto& col_name = table_S.column_names[col_idx];
            if (std::find(table_R.column_names.begin(), table_R.column_names.end(), col_name) == table_R.column_names.end()) {
                row.push_back(table_S.rows[j][col_idx]);
            }
        }
        result.rows.push_back(row);
    }
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "Result table construction time: " << elapsed.count() << " seconds\n";


    // Cleanup
    start = std::chrono::high_resolution_clock::now();
    delete[] h_R_cols;
    delete[] h_S_cols;
    delete[] h_result_indices;
    delete[] h_condition_ops;
    cudaFree(d_condition_ops);
    cudaFree(d_R_cols);
    cudaFree(d_S_cols);
    cudaFree(d_result_indices);
    cudaFree(d_result_count);
    stop = std::chrono::high_resolution_clock::now();
    elapsed = stop - start;
    std::cout << "Cleanup time: " << elapsed.count() << " seconds\n";

    return result;
}