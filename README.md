# Low Latency SPSC Ring Buffer

A lock-free, Single-Producer Single-Consumer (SPSC) ring buffer optimized low latency performace. 

## Benchmark Results

The following results were measured using a 2-thread SPSC setup (1 producer, 1 consumer).


| Ring Size (Entries) | Throughput (Aggregate) | Latency (Mean) | Hardware Cycles* |
| :--- | :--- | :--- | :--- |
| **64 KB** ($2^{16}$) | **879.98 M/s** | **1.136 ns** | ~2.8 cycles |
| **256 KB** ($2^{18}$) | **846.78 M/s** | **1.181 ns** | ~2.9 cycles |
| **1 MB** ($2^{20}$) | **748.63 M/s** | **1.335 ns** | ~3.3 cycles |

*\*Based on 2.5GHz clock (0.4ns per cycle).*

---

## Environment

The benchmark was run under the following conditions:
* **Cloud Provider:** AWS EC2 (`t3.micro`)
* **OS:** Ubuntu 24.04 LTS
* **CPU:** Intel(R) Xeon(R) Platinum 8259CL @ 2.50GHz
* **Topology:** 1 physical core, 2 hyperthreads (SMT)
* **Caches:**
  * **L1 Data:** 32 KiB
  * **L1 Instruction:** 32 KiB
  * **L2 Unified:** 1 MiB
  * **L3 Unified:** 35.8 MiB
* **NUMA Nodes:** 1
* **Virtualization:** KVM (shared cloud hardware)
* 
<img width="590" height="390" alt="latency_cdf" src="https://github.com/user-attachments/assets/a0a97999-2e08-4d7d-85a2-07889caaadab" />

