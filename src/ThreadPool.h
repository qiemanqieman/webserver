#pragma once
#include "pch.h"

#include "TaskQueue.h"

class ThreadPool
{
public:
  ThreadPool(int minNum, int MaxNum);
  ~ThreadPool();
  void addTask(Task& task);
  int getBusyNum();
  int getAliveNum();

private:
  static void* worker(void* arg);
  static void* manager(void* arg);
  void threadExit();
  TaskQueue* taskQueue;
  mutex mtx;
  condition_variable cvNotEmpty;
  thread** workerThreads;
  thread* managerThread;
  int minNum;
  int maxNum;
  int busyNum;
  int aliveNum;
  int exitNum;
  bool shutdown = false;
};