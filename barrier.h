#ifndef CONCURRENCYTS_BARRIER_H
#define CONCURRENCYTS_BARRIER_H

#define __cpp_lib_experimental_barrier 201505

#include <condition_variable>
#include <mutex>

namespace concurrencyts {

template <typename T>
class barrier_base {
public:
  typedef void(*void_func)();
  barrier_base(size_t count) : max_count{count}, count{count}, released_count{0} {}

  void arrive_and_wait() {
    std::unique_lock<std::mutex> ul{lock};
    allow.wait(ul, [this]()->bool { return count > 0; });
  
    if (--count == 0) {
      ul.unlock();
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
      auto ffunc = static_cast<T*>(this)->final_func();
      if (ffunc) {
        ffunc();
      }
      allow.notify_all();
    }
  }

private:
  const size_t max_count;
  std::mutex lock;
  std::condition_variable allow;
  std::condition_variable done;
  size_t count;
  size_t released_count;
};

class barrier : public barrier_base<barrier> {
public:
  barrier(size_t count) : barrier_base{count} {}

  void_func final_func() {
    return nullptr;
  }
};

class flex_barrier : public barrier_base<flex_barrier> {
public:
  flex_barrier(size_t count, void_func fnl_func) : barrier_base{count}, fnl_func{fnl_func} {}

  void_func final_func() {
    return fnl_func;
  }

private:
  void_func fnl_func;
};

}

#endif /* CONCURRENCYTS_BARRIER_H */
