#ifndef CONCURRENCYTS_FUTURE_H
#define CONCURRENCYTS_FUTURE_H

#define __cpp_lib_experimental_future_continuations 201505

#include <cassert>
#include <condition_variable>
#include <forward_list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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
    notify_wait_queue();
  }

  template <typename E>
  void set_exception(E&& exc) {
    std::unique_lock<std::mutex> ul(mtx);
    assert(!is_available && !is_complete);
    eptr = std::make_exception_ptr(std::forward<E>(exc));
    is_complete = true;
    ul.unlock();

    completed.notify_all();
    notify_wait_queue();
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

  bool is_ready() const {
    // this seems thread-unsafe. However, most likely a regular read
    // from an aligned boolean is atomic, and if is_complete is read
    // as true, it won't be false again. At worse, a ready future may
    // indicate it's not ready yet for short time lag. 
    return is_complete;
  }

  void await(std::condition_variable& cv) {
    std::unique_lock<std::mutex> ul(mtx);
    wait_queue.push_front(&cv);
  }

private:
  std::condition_variable completed;
  std::mutex mtx;
  storage_type data;
  bool is_available;
  bool is_complete;
  std::forward_list<std::condition_variable*> wait_queue;
  std::exception_ptr eptr;

  void notify_wait_queue() {
    for (auto*& cv: wait_queue) {
      cv->notify_all();
    }
  }
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

  bool is_ready() const {
    return shared_state->is_ready();
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

  void await(std::condition_variable& cv) {
    shared_state->await(cv);
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

// 
//
template <typename InputIt>
auto when_all(InputIt begin, InputIt end)
  -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>> {
  using ret_type = typename std::iterator_traits<InputIt>::value_type;
  promise<std::vector<ret_type>> prom;
  auto fut = prom.get_future();

  if (begin != end) {
    begin->wait();
    (begin + 1)->wait();
  }

  std::thread t([begin, end](promise<std::vector<ret_type>> prom) {
    try {
      std::vector<ret_type> result;
      begin->wait();
      (begin + 1)->wait();
      for (auto it = begin; it != end; ++it) {
        it->wait();
        result.emplace_back(std::move(*it));
      }
      prom.set(std::move(result));
      std::this_thread::sleep_for(std::chrono::seconds(5));
    } catch(std::exception& e) {
      prom.set_exception(e);
    }
  }, std::move(prom));
  t.detach();

  return fut;
}

template <typename Sequence>
struct when_any_result {
  std::size_t index;
  Sequence futures;
};

template <typename InputIt>
auto make_when_any_result(size_t index, InputIt begin, InputIt end)
  -> when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>
{
  using ret_type = typename std::iterator_traits<InputIt>::value_type; 
  when_any_result<std::vector<ret_type>> result;

  for (auto it = begin; it != end; ++it) {
    result.futures.push_back(std::move(*it));
  }
  result.index = index;

  return result;
}

template <typename InputIt>
auto when_any(InputIt begin, InputIt end)
  -> future<when_any_result<
      std::vector<typename std::iterator_traits<InputIt>::value_type>
  >> {

  using ret_type = when_any_result<
    std::vector<typename std::iterator_traits<InputIt>::value_type>>;
  ret_type result_value;

  struct _sync_context {
    std::mutex mtx;
    std::condition_variable found, start;
    bool done, startCheck;
    promise<ret_type> prom;

    _sync_context() : done(false), startCheck(false) {}
  };
  auto context = std::make_shared<_sync_context>();
  auto result = context->prom.get_future();

  if (begin == end) {
    context->prom.set(make_when_any_result(-1, begin, end));
    return result;
  }

  std::thread thr([begin, end, context]() {
    std::unique_lock<std::mutex> ul{context->mtx};
    context->startCheck = true;
    context->start.notify_one();
    while (!context->done) {
      context->found.wait(ul);

      // main thread found one already
      if (context->done) {
        return;
      }

      for (auto it = begin; it != end; ++it) {
        if (it->is_ready()) {
          context->prom.set(make_when_any_result(std::distance(begin, it),
            begin, end));
          break;
        }
      }
    }
  });
  thr.detach();

  std::unique_lock<std::mutex> ul{context->mtx};
  context->start.wait(ul, [context]()->bool {return context->startCheck;});
  for (auto it = begin; it != end; ++it) {
    if (it->is_ready()) {
      context->done = true;
      context->found.notify_one();
      context->prom.set(make_when_any_result(std::distance(begin, it),
        begin, end));
      break;
    } else {
      it->await(context->found);
    }
  }

  return result;
}

} // namespace concurrencyts

#endif /* CONCURRENCYTS_FUTURE_H */
