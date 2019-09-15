#ifndef CONCURRENCYTS_FUTURE_H
#define CONCURRENCYTS_FUTURE_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace concurrencyts {

  namespace detail {
    template <typename T>
    class future_state {
    public:
      future_state() : is_set{false} {}

      future_state(const future_state&) = delete;
      future_state& operator=(const future_state&) = delete;
      future_state(future_state&&) = delete;
      future_state& operator=(future_state&&) = delete;

      template <typename... Args>
      void emplace(Args&&... args) {
        std::unique_lock<std::mutex> ul(mtx);
        assert(!is_set);

        try {
          new (&data) T{std::forward<Args>(args)...};
        } catch (...) {
          eptr = std::current_exception();
        }

        is_set = true;
        available.notify_all();
      }

      template <typename E>
      void set_exception(E&& exc) noexcept {
        std::unique_lock<std::mutex> ul;
        eptr = std::make_exception_ptr(exc);
        is_set = true;
        available.notify_all();
      }

      ~future_state() {
        reinterpret_cast<T*>(&data)->~T();
      }

      T& get() {
        std::unique_lock<std::mutex> ul(mtx);
        if (!is_set) {
          available.wait(ul, [this]() -> bool { return is_set; });
        }

        if (eptr) {
          std::rethrow_exception(eptr);
        }

        return reinterpret_cast<T&>(data);
      }

      void wait() {
        std::unique_lock<std::mutex> ul(mtx);
        if (!is_set) {
          available.wait(ul, [this]() -> bool { return is_set; });
        }
      }

    protected:
      using storage_type = typename std::aligned_storage<sizeof(T),
        alignof(T)>::type;
      std::mutex mtx;
      std::condition_variable available;
      bool is_set;
      storage_type data;
      std::exception_ptr eptr;
    };
  }

  template <typename T>
  class future {
  public:
    using state_type = detail::future_state<T>;
    future(std::shared_ptr<state_type> state) : shared_state(state) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;
    future(future&&) = default;
    future& operator=(future&&) = default;

    T get() {
      return shared_state->get();
    }

    void wait() { shared_state->wait(); }

    template <typename F, typename... Args>
    auto then(F callable, Args&&... args) 
      -> future<typename std::result_of<F(T, Args...)>::type>
    {
      using ret_type = typename std::result_of<F(T, Args...)>::type;

      std::shared_ptr<concurrencyts::detail::future_state<ret_type>> fs =
        std::make_shared<concurrencyts::detail::future_state<ret_type>>();
      concurrencyts::future<ret_type> fut{fs};

      // promise<ret_type> promise;
      std::thread thr([&fs, &callable, &args...] (future<T> fut) {
        auto val = callable(fut.get(), std::forward<Args>(args)...);
        fs->emplace(val);
        // promise.set(val);
      }, std::move(*this));
      thr.detach();

      // return promise.get_future();
      return fut;
    }

  private:
    std::shared_ptr<state_type> shared_state;
  };

  template <typename T>
  class promise {
  public:
    using state_type = detail::future_state<T>;

    promise() : shared_state{std::make_shared<detail::future_state<T>>()} {}

    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;

    promise(promise&&) = default;
    promise& operator=(promise&&) = default;

    future<T> get_future() {
      // ideally: abort if called more than once
      return future<T>{shared_state};
    }

    void set(T value) {
      // ideally: abort if called more than once
      shared_state->emplace(std::move(value));
    }

    template <typename E>
    void set_exception(E&& exc) noexcept {
      shared_state->set_exception(exc);
    }

  private:
    std::shared_ptr<state_type> shared_state;
  };

  template <typename F, typename... Args>
  auto async(F&& f, Args&&... args) -> future<std::result_of_t<F(Args...)>> {
    using ret_type = std::result_of_t<F(Args...)>;
    promise<ret_type> promise;

    // f and args should really be captured via perfect capture,
    // but since that's not available till C++20, don't want to
    // jump through hoops. So [&f, &args...] works with limitations for now.
    std::thread thr([&promise, &f, &args...] {
      try {
        promise.set(f(std::forward<Args&&>(args)...));
      } catch (std::exception& e) {
        promise.set_exception(e);
      } catch (...) {
        promise.set_exception(std::runtime_error("Unknown exception"));
      }
    });
    thr.detach();
    return promise.get_future();
  }
}

#endif /* CONCURRENCYTS_FUTURE_H */
