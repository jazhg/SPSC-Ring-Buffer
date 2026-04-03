#!/bin/bash
# 1. Compile with aggressive optimizations
g++ -O3 -march=native -flto benchmark.cpp -lbenchmark -lpthread -o rb_bench

# 2. Lock CPU frequency 
echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# 3. Run with hard affinity 
sudo taskset -c 0,1 ./rb_bench --benchmark_out=results/aws_xeon_results.json --benchmark_out_format=json