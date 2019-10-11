#include "future.h"
#include <bits/stdc++.h>

void basic_future_state() {
  concurrencyts::future_state<int> f;

  std::thread t([&f]() {
    auto val = f.get();
    std::cout << val << '\n';
  });

  std::thread t0([&f]() {
    f.wait();
    std::cout << "Done\n";
  });

  std::thread t1([&f]() {
    auto val = f.get();
    std::cout << val << '\n';
  });

  std::this_thread::sleep_for(std::chrono::seconds(2));
  f.emplace(10);

  t.join();
  t0.join();
  t1.join();
}

void promise_to_future() {
  concurrencyts::promise<int> prom;
  auto fut = prom.get_future();

  std::thread t([&prom]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    prom.set(10);
  });
  t.detach();

  std::cout << fut.get() << '\n';
}

void async_to_promise() {
  concurrencyts::future<int> f = concurrencyts::async([](int a, int b, int x)->int {
    std::this_thread::sleep_for(std::chrono::seconds(4));
    return a + b + x;
  }, 10, 20, 30);
  std::cout << f.get() << '\n';

  auto f0 =
  concurrencyts::async([](std::string a, const std::string b, std::string&& x)->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(4));
    return a + " " + b + " " + x;
  }, "hello", "whats'", "up").then([](std::string s, std::string n) ->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return s + n;
  }, "?").then([](std::string s)->int {
    return s.size();
  });
  std::cout << f0.get() << '\n';
}

void when_all() {
  std::vector<concurrencyts::future<std::string>> futures;

  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(4));
    return "Hello";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return "World";
  }) );

  auto rf = concurrencyts::when_all(futures.begin(), futures.end());
  auto result = std::move(rf.get());
  for (auto& r: result) {
    std::cout << "Got: " << r.get() << '\n';
  }
}

void when_any() {
  std::vector<concurrencyts::future<std::string>> futures;

  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return "Hello";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(9));
    return "World";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(8));
    return "I've";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(7));
    return "been";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(6));
    return "waiting";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return "for";
  }) );
  futures.push_back( concurrencyts::async([]()->std::string {
    std::this_thread::sleep_for(std::chrono::seconds(6));
    return "you";
  }) );

  auto rf = concurrencyts::when_any(futures.begin(), futures.end());
  auto result = std::move(rf.get());
  std::cout << result.futures[result.index].get() << '\n';
}

int main() {
  // basic_future_state();
  // promise_to_future();
  // async_to_promise();
  // when_all();
  when_any();
}
