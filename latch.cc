#include "latch.h"

#include <cassert>
#include <iostream>
#include <thread>

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
