# parallel_db

An experimental GPU-accelerated nested-loop join for two CSV tables. The
project explores how a familiar relational operation can be expressed as a
CUDA kernel, from preparing data on the host to collecting matching row pairs
back into a result table.

## CUDA implementation

The host code loads the tables and extracts the join columns into integer
arrays. The CUDA kernel assigns each row of the outer table to a thread and
processes the inner table in shared-memory tiles. Threads evaluate the join
conditions in parallel; warp-level ballot and prefix-count operations group
matching lanes so a warp can reserve output space with a single atomic
operation.

The surrounding C++ code handles CSV parsing, table representation, result
construction, and output. The current GPU path supports integer join columns
and is an evolving prototype rather than a general-purpose database engine.

## Code map

- `src/joins.cu` — CUDA nested-loop join and host-side GPU orchestration.
- `src/headers/gpu_utils.cuh` — device comparison and warp reservation helpers.
- `src/main.cpp` — loads the configured inputs, chooses table roles, invokes
  the join, and saves the result.
- `src/db_utils.cpp` and `src/headers/` — CSV and table utilities, data types,
  configuration declarations, and the host join interface.
- `query.txt` — input paths and output naming settings.
- `scripts/` — small data-generation and CSV-sorting helpers.

## Supporting scripts

`scripts/datagen/cust_order.py` generates 21,000 customer rows and 50,000 order
rows with randomized fields. It writes `tc2_customers.csv` and
`tc2_orders.csv` to the current working directory. Those names differ from the
default paths in `query.txt`; edit the configuration or move the generated
files before using them as inputs. Row counts can be changed near the bottom
of the script.

`scripts/csv_sort.py` reads the filename in its final function call, sorts the
rows numerically by the first column, and writes a file prefixed with
`sorted_`. It currently names `gpu_out_nlj.csv`, while the application writes
`results/nlj_GPU.csv`. Update the example filename to match the output you want
to compare. The script sorts the first column; it does not locate a column by
its header.

## Trying it locally

The active build uses CMake and requires a C++17 compiler, the NVIDIA CUDA
Toolkit (`nvcc`), and a compatible NVIDIA GPU and driver. From the project
root:

```bash
cmake -S . -B build
cmake --build build -j
./program
```

The program reads `query.txt` relative to the current directory. With the
checked-in settings it loads `tables/orders.csv` and `tables/customers.csv`,
then writes `results/nlj_GPU.csv`; the `results/` directory must exist.

This is work in progress: join conditions are currently hard-coded in
`src/main.cpp` rather than read from `query.txt`. The current conditions
include `orders.customer_id == customers.customer_id` and
`orders.customer_id == customers.age`. The query-size settings are not yet
used, and the GPU result buffer is currently sized for 2,000 row pairs.
