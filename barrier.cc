#include "concurrencyts.h"

#include <cassert>
#include <iostream>
#include <thread>


int main() {
  std::experimental::flex_barrier barrier(7, []() {
    std::cout << "All threads done\n";
  });

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
  std::this_thread::sleep_for(std::chrono::seconds(5));
}
