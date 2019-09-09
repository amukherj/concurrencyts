#ifndef CONCURRENCYTS_BARRIER_H
#define CONCURRENCYTS_BARRIER_H

#include <condition_variable>
#include <mutex>

namespace concurrencyts {

class barrier {
public:
  barrier(size_t count);
  void arrive_and_wait();

private:
  std::mutex lock;
  size_t count;
  std::condition_variable done;
};

}

#endif /* CONCURRENCYTS_BARRIER_H */
