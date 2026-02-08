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
  std::atomic<size_t> readIndex{0u}; // The index that is about to be read
  std::size_t size;
  std::vector<T> data;
public:
  Spsc(size_t maxSize) : size(maxSize) { data.reserve(maxSize); }
  bool push(const T& ele)
  {
    size_t curHead = writeIndex.load(std::memory_order_relaxed);
    if (curHead >= size)
      return false;
    data.push_back(ele);
    writeIndex.store(curHead+1, std::memory_order_release);
    return true;
  }
  bool pop(T& ele)
  {
    size_t ind = readIndex.load(std::memory_order_relaxed);
    size_t curHead = writeIndex.load(std::memory_order_acquire);
    if (ind >= curHead)
      return false;
    ele = data[ind];
    readIndex.store(ind+1, std::memory_order_relaxed);
    return true;
  }
  bool front(T& ele)
  {
    size_t ind = readIndex.load(std::memory_order_relaxed);
    size_t curHead = writeIndex.load(std::memory_order_acquire);
    if (ind >= curHead)
      return false;
    ele = data[ind];
    return true;
  }
  bool isFull() { return writeIndex.load(std::memory_order_acquire) >= size; }
};


void consumer(Spsc<int>& spsc)
{
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 3);
  int l = 0;
  for (int i = 0; i < 1000; i++)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(uniform_dist(e1)));
    int ele = 0;
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

void producer(Spsc<int>& spsc, const std::vector<int>& v)
{
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 40);

  for (int i = 0; i < 1000; i++)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(uniform_dist(e1)));
    spsc.push(v[i]);
  }
}


void testRandom()
{
  std::random_device r;
   std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 6);
  for (int i = 0 ; i < 10; i++)
    std::cout << uniform_dist(e1) << std::endl;;
}

int main()
{
  if (false)
    testRandom();
  if (true)
  {
    Spsc<int> spsc(1024u);
    std::vector<int> a(1000, 0);
    for (size_t i = 1; i < a.size(); i++)
      a[i] = a[i-1] + 1;
    std::thread p(producer, std::ref(spsc), std::cref(a));
    std::thread c(consumer, std::ref(spsc));
    c.join(), p.join();
  }
  return 0;
}
