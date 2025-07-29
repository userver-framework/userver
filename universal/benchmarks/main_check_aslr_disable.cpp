#include <benchmark/benchmark.h>
int main() {
    void* p = (void*)&benchmark::MaybeReenterWithoutASLR;
    return 0;
}
