#include <atomic>
using namespace std;

class tas_spinlock
{
    atomic_flag flag{false};

    public:
        
        void lock() {
            while(flag.test_and_set(std::memory_order_acquire)) {
                // loop while lock is hold
            }
        }

        void unlock() {
            flag.clear(std::memory_order_release);
        }

};