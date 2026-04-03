#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_destructive_interference_size;
#else
    static constexpr size_t hardware_destructive_interference_size = 64;
#endif

template<typename T, size_t Size>
class RingBuffer {
private: 
    static_assert((Size & (Size - 1)) == 0, "Only size power of 2 are allowed.");
    static constexpr size_t SpatialPadding = 256;


    /**
     * Element Slot: supports zero-copy construction. 
     */
    struct Slot {
        alignas(T) unsigned char storage[sizeof(T)];
        T* ptr() { return reinterpret_cast<T*>(storage); }
        const T* ptr() const { return reinterpret_cast<const T*>(storage); }
    };

    // Producer state.
    // Contains the tail cache. 
    alignas(SpatialPadding) struct {
        std::atomic<size_t> head{0};
        size_t tail_cache{0};
    } producer_; 

    // Consumer state.
    // Contains the head cache. 
    alignas(SpatialPadding) struct {
        std::atomic<size_t> tail{0};
        size_t head_cache{0};
    } consumer_; 

    // Ring Buffer structure.  
    alignas(SpatialPadding) std::array<Slot, Size> buffer_;


public: 
    RingBuffer() = default;

    ~RingBuffer() {
        while (producer_.head > consumer_.tail) {
            size_t t = consumer_.tail.load(std::memory_order_relaxed);
            buffer_[t & (Size - 1)].ptr()->~T();
            consumer_.tail.store(t + 1, std::memory_order_relaxed);
        }
    }


    //Copy and move operations are disabled to avoid high latency cost. 
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * Emplace: push element onto the ring buffer. 
     * Constructs in-place in the slot.
     */
    template<typename... Args>
    bool emplace(Args&&... args) {
        const size_t head = producer_.head.load(std::memory_order_relaxed);

        // Check if full via tail cache.
        if (head - producer_.tail_cache >= Size) [[unlikely]] {
            producer_.tail_cache = consumer_.tail.load(std::memory_order_acquire);
            if (head - producer_.tail_cache >= Size) return false;
        }

        new (buffer_[head & (Size - 1)].ptr()) T(std::forward<Args>(args)...);
        producer_.head.store(head + 1, std::memory_order_release);
        return true; 
    }

    // Wrappers.
    bool push(const T& item) { return emplace(item); }
    bool push(T&& item)      { return emplace(std::move(item)); }


    /**
     * Consume: pop operation.
     * Direct read access to in-place element, no copy is performed. 
     */
    template<typename F>
    bool consume(F&& callback) {
        const size_t tail = consumer_.tail.load(std::memory_order_relaxed);

        // Check if empty via head cache. 
        if (tail == consumer_.head_cache) [[unlikely]] {
            consumer_.head_cache = producer_.head.load(std::memory_order_acquire);
            if (tail == consumer_.head_cache) return false;
        }

        Slot& slot = buffer_[tail & (Size - 1)];
        T* item_ptr = slot.ptr();

        callback(*item_ptr);

        item_ptr->~T(); 
        consumer_.tail.store(tail + 1, std::memory_order_release);
        return true; 
    }
};