#pragma once

#include <benchmark/benchmark.h>

// Additional macros for google benchmark

// This macro will create a template benchmark with a fixture
#define BENCHMARK_DEFINE_TEMPLATE_F(BaseClass, Method)        \
  template <typename T>                                       \
  class BaseClass##_##Method##_Benchmark : public BaseClass { \
   protected:                                                 \
    virtual void BenchmarkCase(::benchmark::State&);          \
  };                                                          \
  template <typename T>                                       \
  void BaseClass##_##Method##_Benchmark<T>::BenchmarkCase

#define BENCHMARK_INSTANTIATE_TEMPLATE_F(BaseClass, Method, Type) \
  class BaseClass##_##Method##_Benchmark_##Type                   \
      : public BaseClass##_##Method##_Benchmark<Type> {           \
   public:                                                        \
    BaseClass##_##Method##_Benchmark_##Type()                     \
        : BaseClass##_##Method##_Benchmark<Type>() {              \
      this->SetName(#BaseClass "/" #Method "<" #Type ">");        \
    }                                                             \
  };                                                              \
  BENCHMARK_PRIVATE_REGISTER_F(BaseClass##_##Method##_Benchmark_##Type)
