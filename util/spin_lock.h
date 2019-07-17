#pragma once
#include <atomic>
#include <mutex>
namespace Maestro {
    using namespace std;
    
    class SpinLock {
        atomic<bool> _latch;
    public:
        void lock() {
            bool unlatched = false;
            while (!_latch.compare_exchange_weak(unlatched, true, std::memory_order_acquire)) {
                unlatched = false;
            }
        }
        void unlock() {
            _latch.store(false, std::memory_order_release);
        }
    };

    class SpinLockGuard {
        SpinLock& _lock;
    public:
        SpinLockGuard(SpinLock& lock) : _lock(lock) {
            _lock.lock();
        }
        SpinLockGuard(const SpinLockGuard&) = delete;
        SpinLockGuard(SpinLockGuard&&) = delete;
        ~SpinLockGuard() {
            _lock.unlock();
        }
    };
}