#include <atomic>
using namespace std;

class ttas_spinlock
{
    atomic<bool> flag{false};

    public:
        
        void lock() {
            do {
                while(flag.load(std::memory_order_relaxed)) {
                    // loop while lock is hold
                }
            } while(flag.exchange(true, std::memory_order_acquire));
        }

        void unlock() {
            flag.store(false, std::memory_order_release);
        }

};