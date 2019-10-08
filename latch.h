#ifndef CONCURRENCYTS_LATCH_H
#define CONCURRENCYTS_LATCH_H

#include <condition_variable>
#include <mutex>

namespace concurrencyts {

class latch {
public:
  latch(size_t count) : count{count} {}

  void count_down() {
    std::unique_lock<std::mutex> ul{lock};
    assert(count > 0);
    if (--count == 0) {
      ul.unlock();
      done.notify_all();
    }
  }

  void wait() {
    std::unique_lock<std::mutex> ul{lock};
    if (count > 0) {
      done.wait(ul, [this]()->bool { return count == 0; });
    }
  }

private:
  std::mutex lock;
  std::condition_variable done;
  size_t count;
};

}

#endif /* CONCURRENCYTS_LATCH_H */
