#include "TaskQueue.h"

void TaskQueue::addTask(Task &task)
{
  lock_guard<mutex> lock(mtx);
  taskQueue.push(task);
  cout << "add task, current task number is " << taskQueue.size() << endl;
}

Task TaskQueue::takeTask()
{
  lock_guard<mutex> lock(mtx);
  Task task;
  if (taskQueue.empty())
  {
    return task;
  }
  task = taskQueue.front();
  taskQueue.pop();
  cout << "take task, current task number is " << taskQueue.size() << endl;
  return task;
}

size_t TaskQueue::size()
{
  return taskQueue.size();
}
