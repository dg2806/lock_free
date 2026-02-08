#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>

//SPSC
//fixed length
template<typename T>
class Spsc
{
private:
  std::atomic<size_t> writeIndex{0u}; //The index that is avilable for writing
  char padding1[ 248 ]; /* force read_index and write_index to different cache lines */
  std::atomic<size_t> readIndex{0u}; // The index that is about to be read
  char padding2[ 248 ]; /* force read_index and write_index to different cache lines */
  std::size_t size;
  std::vector<T> data;
public:
  Spsc(size_t maxSize) : size(maxSize+1) { data.resize(maxSize+1); }
  bool push(const T& ele)
  {
    size_t curHead = writeIndex.load(std::memory_order_relaxed);
    if (readIndex.load(std::memory_order_relaxed) == (curHead + 1u)%size)
      return false;
    data[curHead] = ele;
    curHead = (curHead+1)%size;
    writeIndex.store(curHead, std::memory_order_release);
    return true;
  }
  bool pop(T& ele)
  {
    size_t ind = readIndex.load(std::memory_order_relaxed);
    size_t curHead = writeIndex.load(std::memory_order_acquire);
    if (ind == curHead)
      return false;
    ele = data[ind];
    ind = (ind+1)%size;
    readIndex.store(ind, std::memory_order_relaxed);
    return true;
  }
  bool front(T& ele)
  {
    size_t ind = readIndex.load(std::memory_order_relaxed);
    size_t curHead = writeIndex.load(std::memory_order_acquire);
    if (ind == curHead)
      return false;
    ele = data[ind];
    return true;
  }
  bool isFull() { return (writeIndex.load(std::memory_order_acquire) + 1u)%size == readIndex.load(std::memory_order_acquire); }
};


void consumer(Spsc<size_t>& spsc, const size_t cnt)
{
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 3);
  size_t l = 0;
  for (size_t i = 0; i < cnt; i++)
  {
//    std::this_thread::sleep_for(std::chrono::microseconds(uniform_dist(e1)));
    size_t ele = 0;
    bool success = spsc.pop(ele);
    if (success)
    {
      if (l != ele)
        std::cout << "failure " << i << " " << ele << " " << l << std::endl; 
      l++;
    }
  }
  std::cout << "consumed " << l << std::endl;
}

void producer(Spsc<size_t>& spsc, const size_t cnt)
{
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 20);
  int l = 0;
  for (size_t i = 0; i < cnt; i++)
  {
//    std::this_thread::sleep_for(std::chrono::microseconds(uniform_dist(e1)));
    bool success = spsc.push(l);
    if (success)
      l++;
  }
}


void testRandom()
{
  std::vector<int> a(1000, 0);
  auto start = std::chrono::high_resolution_clock::now();
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 6);
  for (size_t i = 0 ; i < 1000; i++)
    a[i] = uniform_dist(e1);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
  }

int main()
{
  if (false)
    testRandom();
  if (true)
  {
    Spsc<size_t> spsc(1024u);
    size_t cnt = 1024*1024;
    std::vector<size_t> a(cnt, 0);
    for (size_t i = 1; i < a.size(); i++)
      a[i] = a[i-1] + 1;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread p(producer, std::ref(spsc), cnt);
    std::thread c(consumer, std::ref(spsc), cnt);
    c.join(), p.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
  }
  return 0;
}
