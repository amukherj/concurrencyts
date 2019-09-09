#include "barrier.h"

#include <cassert>
#include <iostream>
#include <thread>

namespace concurrencyts {

barrier::barrier(size_t count) : lock{}, count{count} {}

void barrier::arrive_and_wait() {
  std::unique_lock<std::mutex> ul{lock};
  assert(count > 0);
  if (--count == 0) {
    done.notify_all();
  } else {
    while (count > 0) {
      done.wait(ul, [this]()->bool { return count == 0; });
    }
  }
}

}

int main() {
  concurrencyts::barrier barrier(7);
  for (int i = 0; i < 5; ++i) {
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

  barrier.arrive_and_wait();
  std::cout << "All threads done\n";
  thr1.join();
}
