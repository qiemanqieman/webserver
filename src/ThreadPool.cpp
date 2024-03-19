#include "ThreadPool.h"
#include <unistd.h>

ThreadPool::ThreadPool(int minNum, int MaxNum)
    : minNum(minNum), maxNum(maxNum)
{
  taskQueue = new TaskQueue();
  do
  {
    busyNum = 0;
    aliveNum = minNum;
    exitNum = 0;
    workerThreads = new std::thread *[maxNum];
    if (workerThreads == nullptr)
    {
      cerr << "malloc thread array failed..." << endl;
      break;
    }
    memset(workerThreads, 0, sizeof(std::thread *) * maxNum);

    // create threads
    for (int i = 0; i < minNum; ++i)
    {
      cout << "create thread ..." << endl;
      workerThreads[i] = new std::thread(&worker, this);
    }
    managerThread = new std::thread(&manager, this);
  } while (0);
}

ThreadPool::~ThreadPool()
{
  shutdown = true;
  managerThread->join();
  cout << "manager thread " << managerThread->get_id()
       << " join/destroy success" << endl;
  cvNotEmpty.notify_all();
  cout << "worker thread notify all" << endl;
  if (taskQueue)
  {
    delete taskQueue;
    taskQueue = nullptr;
  }
  if (workerThreads)
  {
    delete[] workerThreads;
    workerThreads = nullptr;
  }
  if (managerThread)
  {
    delete managerThread;
    managerThread = nullptr;
  }
  cout << "thread pool destroy success" << endl;
}

void ThreadPool::addTask(Task &task)
{
  if (shutdown)
  {
    cout << "thread pool has been shutdown, can't add task" << endl;
    return;
  }
  cout << "add task to pool..." << endl;
  taskQueue->addTask(task);
  cvNotEmpty.notify_one();
}

int ThreadPool::getBusyNum()
{
  auto l = lock_guard<mutex>(mtx);
  return busyNum;
}

int ThreadPool::getAliveNum()
{
  auto l = lock_guard<mutex>(mtx);
  return aliveNum;
}

void *ThreadPool::worker(void *arg)
{
  auto pool = static_cast<ThreadPool *>(arg);
  while (true)
  {
    unique_lock<mutex> lock(pool->mtx);
    while (pool->taskQueue->size() == 0 and not pool->shutdown)
    {
      cout << "worker thread " << std::this_thread::get_id() << " is waiting..." << endl;
      pool->cvNotEmpty.wait(lock);
      if (pool->exitNum > 0)
      {
        // there are some threads need to be destroyed
        pool->aliveNum--;
        lock.unlock();
        pool->threadExit();
      }
    }
    if (pool->shutdown)
    {
      lock.unlock();
      pool->threadExit();
    }

    auto task = pool->taskQueue->takeTask();
    pool->busyNum++;
    lock.unlock();
    cout << "worker thread " << std::this_thread::get_id() << " start working..." << endl;
    task.function(task.arg);
    cout << "worker thread " << std::this_thread::get_id() << " finish working..." << endl;
    lock.lock();
    pool->busyNum--;
    lock.unlock();
  }
  return nullptr;
}

void *ThreadPool::manager(void *arg)
{
  auto pool = static_cast<ThreadPool *>(arg);
  while (not pool->shutdown)
  {
    this_thread::sleep_for(chrono::seconds(1));
    unique_lock<mutex> lock(pool->mtx);
    int queueSize = pool->taskQueue->size();
    int liveNum = pool->aliveNum;
    int busyNum = pool->busyNum;
    lock.unlock();

    const int NUMBER = 2; // add 2 worker threads at once
    if (queueSize > liveNum and liveNum < pool->maxNum)
    {
      lock.lock();
      for (int i = 0, num = 0; i < pool->maxNum and num < NUMBER /*and pool->aliveNum < pool->maxNum*/; ++i)
      {
        if (pool->workerThreads[i] == nullptr or not pool->workerThreads[i]->joinable())
        {
          pool->workerThreads[i] = new std::thread(&worker, pool);
          ++pool->aliveNum;
          ++num;
        }
      }
      lock.unlock();
    }
    // destroy rest threads
    if (busyNum * 2 < liveNum and liveNum > pool->minNum)
    {
      lock.lock();
      pool->exitNum = NUMBER;
      lock.unlock();
      for (int i = 0; i < NUMBER; ++i)
      {
        pool->cvNotEmpty.notify_one();
      }
    }
  }
  return nullptr;
}

void ThreadPool::threadExit()
{
  auto tid = std::this_thread::get_id();
  for (int i = 0; i < maxNum; ++i)
  {
    if (workerThreads[i] and workerThreads[i]->get_id() == tid)
    {
      cout << "threadExit() :thread " << tid << " exit..." << endl;
      delete workerThreads[i];
      workerThreads[i] = nullptr;
      break;
    }
  }
  #ifdef __linux__
    // Code for Linux platform
    pthread_exit(nullptr);
  #elif __WIN32__
    // Code for Windows platform
    _endthreadex(0);
  #elif __APPLE__
    // Code for macOS platform
  #else
    // Code for other platforms
  #endif

}
