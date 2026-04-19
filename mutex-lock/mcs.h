#include <atomic>

struct mcs_node {
    std::atomic<mcs_node*> next;
    std::atomic<bool> waiting;   // true = đang đợi, false = được vào
};

class mcs_lock {
    std::atomic<mcs_node*> tail{nullptr};   // trỏ tới tail của queue

public:
    void lock(mcs_node* my) {
        my->next.store(nullptr, std::memory_order_relaxed);
        my->waiting.store(true, std::memory_order_relaxed);

        // enqueue: đưa myself vào cuối queue, nhận về predecessor
        mcs_node* prev = tail.exchange(my, std::memory_order_acq_rel);

        if (prev != nullptr) {
            // nối vào sau predecessor
            prev->next.store(my, std::memory_order_release);
            // spin trên cờ của CHÍNH MÌNH
            while (my->waiting.load(std::memory_order_acquire)) {
                // busy-wait local
            }
        }
        // nếu prev == nullptr: queue rỗng, mình giữ lock luôn
    }

    void unlock(mcs_node* my) {
        mcs_node* succ = my->next.load(std::memory_order_acquire);
        if (succ == nullptr) {
            // không thấy successor → thử set tail = nullptr
            mcs_node* expected = my;
            if (tail.compare_exchange_strong(
                    expected, nullptr,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                return; // không ai chờ
            }
            // có ai đó đang enqueue nhưng chưa link next
            do {
                succ = my->next.load(std::memory_order_acquire);
            } while (succ == nullptr);
        }
        // đánh thức successor
        succ->waiting.store(false, std::memory_order_release);
    }
};