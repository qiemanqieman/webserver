#pragma once
#include "pch.h"

using callback = void *(*)(void *);

struct Task
{
  Task() : function(nullptr), arg(nullptr) {}
  Task(callback f, void *a) : function(f), arg(a) {}
  callback function;
  void *arg;
};

class TaskQueue
{
  public:
  TaskQueue() {}
  ~TaskQueue() {}
  void addTask(Task &task);
  Task takeTask();
  size_t size();

private:
  mutex mtx;
  queue<Task> taskQueue;
};

