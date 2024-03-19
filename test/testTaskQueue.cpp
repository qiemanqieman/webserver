#include "TaskQueue.h"

#include<gtest/gtest.h>
 
int add(int a,int b)
{
    return a+b;
}
 
TEST(testCase,test0)
{
    EXPECT_EQ(add(2,3),5);
}

TEST(TaskQueueTest, AddTaskTest)
{
  TaskQueue taskQueue;
  Task task1{};
  Task task2{};

  taskQueue.addTask(task1);
  EXPECT_EQ(taskQueue.size(), 1);

  taskQueue.addTask(task2);
  EXPECT_EQ(taskQueue.size(), 2);
}