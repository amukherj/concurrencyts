#include "future.h"
#include <bits/stdc++.h>
// using namespace std;

double foo(int arg, int f1) {
  return 3.14159259 + arg + f1;
}

struct Foo {
  double operator()(int arg, int f1) {
    return 3.14159259 + arg + f1;
  }
};

int main() {
  concurrencyts::promise<int> promise;
  auto f = promise.get_future();

  std::thread thr([&promise] {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    promise.set(10);
  });
  thr.detach();

  f.wait();
  std::cout << f.then(Foo{}, 20).get() << '\n';
}
