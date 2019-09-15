#ifndef CONCURRENCYTS_FUTURE_H
#define CONCURRENCYTS_FUTURE_H

#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace concurrencyts {

// shared state for futures and promises
template <typename T>
class future_state {
public:
  using storage_type = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

  future_state() : is_available(false), is_complete(false) {}

  future_state(const future_state& that) = delete; 
  future_state& operator=(const future_state& that) = delete; 
  future_state(future_state&& that) = delete; 
  future_state& operator=(future_state&& that) = delete; 

  ~future_state() {
    if (is_available) {
      reinterpret_cast<T*>(&data)->~T();
    }
  }

  // setters
  template <typename... Args>
  void emplace(Args&&... args) {
    std::unique_lock<std::mutex> ul(mtx);
    assert(!is_available && !is_complete);
    try {
      new (reinterpret_cast<T*>(&data)) T{std::forward<Args&&>(args)...};
      is_available = true;
    } catch (...) {
      eptr = std::current_exception();
    }
    is_complete = true;
    ul.unlock();

    completed.notify_all();
  }

  template <typename E>
  void set_exception(E&& exc) {
    std::unique_lock<std::mutex> ul(mtx);
    assert(!is_available && !is_complete);
    eptr = std::make_exception_ptr(std::forward<E>(exc));
    is_complete = true;
    ul.unlock();

    completed.notify_all();
  }

  // getters
  T& get() {
    std::unique_lock<std::mutex> ul(mtx);
    completed.wait(ul, [this]()->bool { return is_complete; });

    if (eptr) {
      std::rethrow_exception(eptr);
    }

    return *(reinterpret_cast<T*>(&data));
  }

  void wait() {
    std::unique_lock<std::mutex> ul(mtx);
    completed.wait(ul, [this]()->bool { return is_complete; });

    if (eptr) {
      std::rethrow_exception(eptr);
    }
  }

private:
  std::condition_variable completed;
  std::mutex mtx;
  storage_type data;
  bool is_available;
  bool is_complete;
  std::exception_ptr eptr;
};

template <typename T>
class future {
public:
  future(std::shared_ptr<future_state<T>> state) : shared_state(state) {}

  future(const future&) = delete;
  future& operator=(const future&) = delete;
  future(future&&) = default;
  future& operator=(future&&) = default;

  void wait() {
    shared_state->wait();
  }

  T& get() {
    return shared_state->get();
  }

  template <typename F, typename... Args>
  auto then(F&& callable, Args&&... args) ->
      future<std::result_of_t<F(T, Args&&...)>>
  {
    using ret_type = std::result_of_t<F(T, Args&&...)>;
    auto next_state = std::make_shared<future_state<ret_type>>();
    future<ret_type> ret(next_state);

    std::thread t([ss = next_state, &callable, &args...](future<T> f) {
      try {
        ss->emplace(callable(f.get(), std::forward<Args&&>(args)...));
      } catch (std::exception& e) {
        ss->set_exception(e);
      }
    }, std::move(*this));
    t.detach();

    return ret;
  }

private:
  std::shared_ptr<future_state<T>> shared_state;
};

template <typename T>
class promise {
public:
  promise() : shared_state(std::make_shared<future_state<T>>()) {}

  promise(const promise&) = delete;
  promise& operator=(const promise&) = delete;
  promise(promise&&) = default;
  promise& operator=(promise&&) = default;

  future<T> get_future() {
    return future<T>(shared_state);
  }

  template <typename... Args>
  void set(Args&&... args) {
    shared_state->emplace(std::forward<Args&&>(args)...);
  }

  template <typename E>
  void set_exception(E&& exc) {
    shared_state->set_exception(std::forward<E>(exc));
  }

private:
  std::shared_ptr<future_state<T>> shared_state;
};

template <typename F, typename... Args>
auto async(F&& callable, Args&&... args) -> 
    future<typename std::result_of_t<F(Args&&...)>>
{
  using ret_type = typename std::result_of_t<F(Args&&...)>;
  promise<ret_type> prom;
  auto fut = prom.get_future();

  std::thread t([&callable, &args...](promise<ret_type> promise) {
    try {
      promise.set(callable(std::forward<Args&&>(args)...));
    } catch (std::exception& e) {
      promise.set_exception(e);
    }
  }, std::move(prom));
  t.detach();

  return fut;
}

} // namespace concurrencyts

#endif /* CONCURRENCYTS_FUTURE_H */
