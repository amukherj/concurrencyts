#ifndef CONCURRENCYTS_BARRIER_H
#define CONCURRENCYTS_BARRIER_H

#define __cpp_lib_experimental_barrier 201505

#include <condition_variable>
#include <mutex>

namespace concurrencyts {

template <typename T>
class barrier_base {
public:
  typedef int(*compl_func)();
  barrier_base(size_t count) : max_count{count}, count{count}, released_count{0} {}

  void arrive_and_wait() {
    std::unique_lock<std::mutex> ul{lock};
    allow.wait(ul, [this]()->bool { return count > 0; });
  
    if (--count == 0) {
      ul.unlock();

      auto ffunc = static_cast<T*>(this)->final_func();
      if (ffunc) {
        // run the completion block
        auto tmp_count = ffunc();
        if (tmp_count >= 0) {
          // readjust max_count for next phase of barrier.
          max_count = tmp_count;
        }
      }

      // release all waiting threads
      done.notify_all();
    } else {
      done.wait(ul, [this]()->bool { return count == 0; });
      ul.unlock();
    }
  
    ul.lock();
    if (++released_count == max_count) {
      count = max_count;
      released_count = 0;
      ul.unlock();
      // allow all waiting threads
      allow.notify_all();
    }
  }

private:
  size_t max_count;
  std::mutex lock;
  std::condition_variable allow;
  std::condition_variable done;
  size_t count;
  size_t released_count;
};

class barrier : public barrier_base<barrier> {
public:
  barrier(size_t count) : barrier_base{count} {}

  compl_func final_func() {
    return nullptr;
  }
};

class flex_barrier : public barrier_base<flex_barrier> {
public:
  flex_barrier(size_t count, compl_func fnl_func) : barrier_base{count}, fnl_func{fnl_func} {}

  compl_func final_func() {
    return fnl_func;
  }

private:
  compl_func fnl_func;
};

}

#endif /* CONCURRENCYTS_BARRIER_H */
