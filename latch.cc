#include "latch.h"

#include <cassert>
#include <iostream>
#include <thread>

namespace concurrencyts {

latch::latch(size_t count) : count{count} {}

void latch::count_down() {
  std::unique_lock<std::mutex> ul{lock};
  assert(count > 0);
  if (--count == 0) {
    ul.unlock();
    done.notify_all();
  }
}

void latch::wait() {
  std::unique_lock<std::mutex> ul{lock};
  if (count > 0) {
    done.wait(ul, [this]()->bool { return count == 0; });
  }
}

}

int main() {
  concurrencyts::latch latch(7);
  for (int i = 0; i < 7; ++i) {
    std::thread thr([&, i]() {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::cout << "Thread " << i << " done\n";
      latch.count_down();
    });
    thr.detach();
  }

  latch.wait();
  std::cout << "Latch opened\n";
}
