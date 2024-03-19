#pragma once

#include "pch.h"

#include "ThreadPool.h"
#include "TaskQueue.h"

struct FdInfo{
  int fd;
  int epfd;
  thread::id tid;
};

class Server{
public:
  Server();
  ~Server();

  int run(int argc, char* argv[]);

private:
  int initListenFd(unsigned short port);
  int epollRun(int fd);

private:
  ThreadPool* pool;
};