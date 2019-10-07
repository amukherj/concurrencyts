#include "barrier.h"

#include <cassert>
#include <iostream>
#include <thread>

namespace concurrencyts {

barrier::barrier(size_t count) : max_count{count}, count{count}, released_count{0} {}

void barrier::arrive_and_wait() {
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
    allow.notify_all();
  }
}

}

int main() {
  concurrencyts::barrier barrier(7);
  for (int i = 0; i < 6; ++i) {
    std::thread thr([&, i]() {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      barrier.arrive_and_wait();
      std::cout << "Thread " << i << " done\n";
    });
    thr.detach();
  }

  std::thread thr1([&]() {
    barrier.arrive_and_wait();
    std::cout << "thr1 done\n";
  });
  thr1.join();

  for (int i = 0; i < 6; ++i) {
    std::thread thr([&, i]() {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      barrier.arrive_and_wait();
      std::cout << "Thread " << i << " done\n";
    });
    thr.detach();
  }
  barrier.arrive_and_wait();
  std::cout << "All threads done\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));
}
