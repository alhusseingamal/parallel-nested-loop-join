#include <string>
#include <algorithm>
#include <type_traits>
#include <variant>
#include "db_utils.h"

#define WARP_SIZE 32

// Device function to perform comparisons
template <typename T>
__device__ bool compare_values(T a, T b, CompareOp op) {
    switch (op) {
        case CompareOp::EQ: return a == b;
        case CompareOp::NE: return a != b;
        case CompareOp::LT: return a < b;
        case CompareOp::GT: return a > b;
        case CompareOp::LE: return a <= b;
        case CompareOp::GE: return a >= b;
        default: return false;
    }
}

void copy_column_to_array(const Table& table, const int col_index, int* arr) {
    size_t row_count = table.rows.size();
    for (size_t i = 0; i < row_count; ++i) {
        const DataType& value = table.rows[i][col_index];
        if (std::holds_alternative<int>(value)) {
            arr[i] = std::get<int>(value);
        } else {
            throw std::runtime_error("Column contains non-integer values!");
        }
    }
}

// Returns the mask of lanes whose lane-id is smaller than "me" *inside this warp*.
// Works for 32-lane warps; switch to 64-bit if you ever target 64-lane devices.
__device__ __forceinline__
unsigned int lane_mask_lt() {
    unsigned int me = threadIdx.x & (WARP_SIZE - 1);
    return (1u << me) - 1u;
}

// Reserve <n> consecutive slots in the global output array using a single atomic
// performed by the warp leader, then broadcast the base index to the warp.
__device__ __forceinline__
unsigned int warp_reserve(int n, int *counter /*global*/) {
    unsigned int mask   = __activemask();        // all lanes that reached this call
    int          leader = __ffs(mask) - 1;       // first active lane
    int          base   = 0;
    if ((threadIdx.x & (WARP_SIZE - 1)) == leader)
        base = atomicAdd(counter, n);            // single global atomic
    base = __shfl_sync(mask, base, leader);      // broadcast to the warp (each thread with a set bit in mask receives the value base from leader)
    return base;
}
