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
  // concurrencyts::future<int> f{10};
  std::shared_ptr<concurrencyts::detail::future_state<int>> fs =
    std::make_shared<concurrencyts::detail::future_state<int>>();
  concurrencyts::future<int> f{fs};

  std::thread thr([fs] {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    fs->emplace(10);
  });
  thr.detach();

  f.wait();
  std::cout << f.then(Foo{}, 20).get() << '\n';

  /* cout << f.then(foo, 20).get() << '\n';
  cout << f.then([](int arg, int f1) ->double { return 3.14159259 + arg + f1; }, 20).get() << '\n';
  cout << f.then(Foo{}, 20).get() << '\n'; */
}
