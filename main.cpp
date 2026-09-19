#include <iostream>
#include <cstddef>
#include <random>
#include <vector>
#include <pthread.h>

bool isInside(double x, double y, double r) {
  return x * x + y * y <= r * r;
}

size_t calc(double r, size_t tests, size_t seed) {
  std::default_random_engine engine(seed);
  std::uniform_real_distribution< double > dist(-r, r);
  size_t pass = 0;
  for (size_t i = 0; i < tests; ++i) {
    double x = dist(engine);
    double y = dist(engine);
    if (isInside(x, y, r)) {
      ++pass;
    }
  }
  return pass;
}

struct Args {
  double r;
  size_t tests;
  size_t seed;
  size_t result;
  std::exception_ptr eptr;
};

void * threadFunc(void * arg) {
  Args * args = static_cast< Args * >(arg);
  try {
    args->result = calc(args->r, args->tests, args->seed);
  } catch (...) {
    args->eptr = std::current_exception();
  }
  return nullptr;
}

class ThreadGuard {
  public:
    ThreadGuard(std::vector< pthread_t > & tids, size_t & created):
      tids_(tids),
      created_(created)
    {}
    ~ThreadGuard()
    {
      for (size_t i = 0; i < created_; ++i)
      {
        pthread_join(tids_[i], nullptr);
      }
    }
    ThreadGuard(const ThreadGuard &) = delete;
    ThreadGuard & operator=(const ThreadGuard &) = delete;

  private:
    std::vector< pthread_t > & tids_;
    size_t & created_;
};

double area(double r, size_t threads, size_t tests)
{
  if (threads == 0) {
    throw std::invalid_argument("threads must be > 0");
  }
  size_t base = tests / threads;
  size_t rem  = tests % threads;
  std::vector< pthread_t > tids(threads);
  std::vector< Args > args(threads);
  size_t created = 0;
  ThreadGuard guard(tids, created);
  for (size_t i = 0; i < threads; ++i) {
    size_t my_tests = base + (i < rem ? 1 : 0);
    args[i].r = r;
    args[i].tests = my_tests;
    args[i].seed = 12345 + i;
    args[i].result = 0;
    args[i].eptr = nullptr;
    int err = pthread_create(&tids[i], nullptr, threadFunc, &args[i]);
    if (err != 0) {
      throw std::runtime_error(std::strerror(err));
    }
    ++created;
  }
  for (size_t i = 0; i < threads; ++i) {
    int err = pthread_join(tids[i], nullptr);
    if (err != 0) {
      throw std::runtime_error(std::strerror(err));
    }
  }
  for (size_t i = 0; i < threads; ++i) {
    if (args[i].eptr) {
      std::rethrow_exception(args[i].eptr);
    }
  }
  size_t pass = 0;
  for (size_t i = 0; i < threads; ++i) {
    pass += args[i].result;
  }
  return static_cast< double >(pass) / static_cast< double >(tests) * 4.0 * r * r;
}

int main() {
  double r = 5.0;
  size_t threads = 16;
  size_t tests = 100000;
  try {
    double origArea = 3.14 * r * r;
    double monteArea = area(r, threads, tests);
    std::cout << "Обычная площадь = " << origArea << '\n';
    std::cout << "Площадь монте = " << monteArea << '\n';
  } catch (const std::exception & e) {
    std::cerr << "Ошибка: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
