#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <thread>

#include "ring_buffer.h"

// Portable CPU relax
inline void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    asm volatile("yield");
#else
    std::this_thread::yield();
#endif
}

// ---------------- BARRIER ----------------

class Barrier {
public:
    void wait() {
        int gen = generation_.load(std::memory_order_acquire);

        if (count_.fetch_add(1, std::memory_order_acq_rel) == 1) {
            count_.store(0, std::memory_order_release);
            generation_.fetch_add(1, std::memory_order_release);
        } else {
            while (generation_.load(std::memory_order_acquire) == gen) {
                cpu_relax();
            }
        }
    }

private:
    alignas(64) std::atomic<int> count_{0};
    alignas(64) std::atomic<int> generation_{0};
};

// ---------------- SHARED ----------------

template <std::size_t RingSize>
struct Shared {
    RingBuffer<uint64_t, RingSize> ring;
    alignas(64) Barrier barrier;
};

// ---------------- BENCH ----------------

template <std::size_t RingSize>
static void BM_SPSC(benchmark::State& state) {
    if (state.threads() != 2) {
        state.SkipWithError("Requires exactly 2 threads");
        return;
    }

    static Shared<RingSize> shared;
    const std::size_t items = state.range(0);

    for (auto _ : state) {
        shared.barrier.wait();

        if (state.thread_index() == 0) {
            // ---------------- PRODUCER ----------------
            std::size_t produced = 0;

            while (produced < items) {
                if (shared.ring.emplace(produced)) {
                    ++produced;
                } else {
                    cpu_relax();
                }
            }

        } else {
            // ---------------- CONSUMER ----------------
            std::size_t consumed = 0;

            while (consumed < items) {
                if (shared.ring.consume([](uint64_t) {})) {
                    ++consumed;
                } else {
                    cpu_relax();
                }
            }
        }

        shared.barrier.wait();

        shared.barrier.wait(); 
    }

    if (state.thread_index() == 0) {
        state.counters["throughput_items_per_sec"] =
            benchmark::Counter(static_cast<double>(items),
                               benchmark::Counter::kIsIterationInvariantRate);

        state.counters["avg_sec_per_item"] =
            benchmark::Counter(static_cast<double>(items),
                               benchmark::Counter::kIsIterationInvariantRate |
                               benchmark::Counter::kInvert);

        state.counters["ring_size"] = RingSize;
    }
}

// ---------------- BENCHES ----------------

BENCHMARK_TEMPLATE(BM_SPSC, 1 << 16)
    ->Arg(100'000'000)
    ->Threads(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->MinTime(1.0)
    ->Repetitions(3)
    ->ReportAggregatesOnly();

BENCHMARK_TEMPLATE(BM_SPSC, 1 << 18)
    ->Arg(100'000'000)
    ->Threads(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->MinTime(1.0)
    ->Repetitions(3)
    ->ReportAggregatesOnly();

BENCHMARK_TEMPLATE(BM_SPSC, 1 << 20)
    ->Arg(100'000'000)
    ->Threads(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->MinTime(1.0)
    ->Repetitions(3)
    ->ReportAggregatesOnly();

BENCHMARK_MAIN();