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
  const size_t max_count;
  std::mutex lock;
  std::condition_variable allow;
  std::condition_variable done;
  size_t count;
  size_t released_count;
};

}

#endif /* CONCURRENCYTS_BARRIER_H */
