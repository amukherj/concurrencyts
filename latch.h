#ifndef CONCURRENCYTS_LATCH_H
#define CONCURRENCYTS_LATCH_H

#include <condition_variable>
#include <mutex>

namespace concurrencyts {

class latch {
public:
  latch(size_t count);
  void count_down();
  void wait();

private:
  std::mutex lock;
  std::condition_variable done;
  size_t count;
};

}

#endif /* CONCURRENCYTS_LATCH_H */
