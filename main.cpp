#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>

template<typename T>
class Mpmc
{
private:
  struct Node
  {
    T val;
    Node* next;
  };
  Node* root;
  char padding1[248];
  std::atomic<Node*> head{nullptr};
  char padding2[248];
  std::atomic<Node*> tail{nullptr};
public:
  Mpmc()
  : root{new Node{}}
  , head{root}
  , tail{root}
  {
  };
  bool push(const T& t)
  {
    Node* next = new Node();
    next->val = t;
    next->next = nullptr;
    Node* prev = head.exchange(next, std::memory_order_acq_rel);
    prev->next = next;
    return true;
  }
  bool pop(T& t)
  {
    Node* oldVal; Node* n;
    do
    {
      oldVal = tail.load(std::memory_order_acquire);
      if (oldVal == head.load(std::memory_order_acquire) || oldVal->next == nullptr)  return false;
      n = oldVal->next;
    } while (!tail.compare_exchange_strong(oldVal, n));
    t = n->val;
    return true;
  }
};


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
    readIndex.store(ind, std::memory_order_release);
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
  if (false)
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
  if (true)
  {
    Mpmc<size_t> mpmc;
    size_t cnt = 1024*1024;
    std::vector<size_t> consumed(cnt, 0);
    std::atomic<size_t> consumCnt{0u};
    std::atomic<size_t> produceCnt{0u};
    size_t num = cnt / 4;
    auto produce = [&]()
    {
      for (size_t i = 0; i < num; i++)
      {
        size_t toInsert = produceCnt.fetch_add(1);
        mpmc.push(toInsert);
      }
    };
    auto consumeAndRecord = [&]()
    {
      for (size_t i = 0; i < num; i++)
      {
        size_t val;
        if (mpmc.pop(val))
        {
          size_t loc = consumCnt.fetch_add(1);
          consumed[loc] = val;
        }
      }
    };
    std::thread p1(produce);
    std::thread p2(produce);
    std::thread p3(produce);
    std::thread c1(consumeAndRecord);
    std::thread c2(consumeAndRecord);
    std::thread c3(consumeAndRecord);
    auto startTime = std::chrono::high_resolution_clock::now();
    c1.join(), p1.join(), c2.join(), p2.join(), c3.join(), p3.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - startTime);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "produce cnt " << produceCnt.load() << std::endl;
    std::cout << "consum cnt " << consumCnt.load() << std::endl;
    std::sort(consumed.begin(), consumed.end());
    size_t start = 0;
    while (start < consumed.size() && consumed[start] == 0)
      start++;
    for (size_t i = start+1 ; i < consumed.size()-3; i++)
    {
      if (consumed[i] != consumed[i-1]+1)
        std::cout << "error at i " << consumed[i] << " " << consumed[i-1] << std::endl;
    }
  }
  return 0;
}
