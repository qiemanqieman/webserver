#include "Server.h"

static void *acceptClient(void *arg);
static void *recvHttpRequest(void *arg);

static int parseRequestLine(const char *line, int cfd);
static int sendHeadMsg(int cfd, int status, const char *descr,
                       const char *type, int length);
static int sendFile(const char *fileName, int cfd);
static int sendDir(const char *dirName, int cfd);
static const char *getFileType(const char *name);
static int hexToDec(char c);
static void decodeMsg(char *to, char *from);

Server::Server()
{
  pool = new ThreadPool(8, 8);
}

Server::~Server()
{
  if (pool)
    delete pool, pool = nullptr;
}

int Server::run(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Please input : ./xxx port path\n");
    return -1;
  }
  unsigned short port = atoi(argv[1]);
  // chdir(argv[2]);
  cout << "current path BEFORE change dir: " << filesystem::current_path() << endl;
  filesystem::current_path(argv[2]); // ~/0tmp/webserver/build/src/../..
  cout << "current path AFTER change dir: " << filesystem::current_path() << endl;

  cout << "Init listen..." << endl;
  int lfd = initListenFd(port);
  if (lfd == -1)
    return -1;

  cout << "epoll run..." << endl;
  epollRun(lfd);

  return 0;
}

int Server::initListenFd(unsigned short port)
{
  // 1. create socket
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1)
  {
    perror("socket error");
    return -1;
  }

  // 2. set address reuse
  int opt = 1;
  int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret == -1)
  {
    perror("setsockopt error");
    return -1;
  }

  // 3. bind the addr
  // struct sockaddr_in serv;
  sockaddr_in serv;
  bzero(&serv, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_port = htons(port);
  serv.sin_addr.s_addr = htonl(INADDR_ANY);
  int flag = 1;
  ret = bind(lfd, (struct sockaddr *)&serv, sizeof(serv));
  if (ret == -1)
  {
    perror("bind error");
    return -1;
  }

  // 4. listen
  ret = listen(lfd, 128);
  if (ret == -1)
  {
    perror("listen error");
    return -1;
  }

  return lfd;
}

int Server::epollRun(int lfd)
{
  // 1. create a epoll instance
  int epfd = epoll_create(1);
  if (epfd == -1)
  {
    perror("epoll_create error");
    return -1;
  }

  // 2. register lfd to epoll
  struct epoll_event ev;
  ev.data.fd = lfd;
  ev.events = EPOLLIN;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
  if (ret == -1)
  {
    perror("epoll_ctl create error");
    return -1;
  }

  // loop to check
  struct epoll_event evs[1024];
  int size = sizeof(evs) / sizeof(struct epoll_event);
  struct FdInfo info;
  while (1)
  {
    int num = epoll_wait(epfd, evs, size, -1);
    for (int i = 0; i < num; ++i)
    {
      int fd = evs[i].data.fd;
      info.fd = fd;
      info.epfd = epfd;
      if (fd == lfd)
      {
        // accept
        acceptClient(&info);
        cout << "acceptClient..." << endl;
        usleep(1);
      }
      else
      {
        printf("recvHttpRequest...\n");

        auto task = Task(recvHttpRequest, &info);
        pool->addTask(task);
        usleep(1);
      }
    }
  }
}

static void *acceptClient(void *arg)
{
  cout << "acceptClient thread id... " << this_thread::get_id() << endl;
  auto info = (struct FdInfo *)arg;
  // 1. establish connection
  int cfd = accept(info->fd, nullptr, nullptr);
  if (cfd == -1)
  {
    perror("accept error");
    return nullptr;
  }

  // set cfd is noblock
  int flag = fcntl(cfd, F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(cfd, F_SETFL, flag);

  // add cfd to epoll
  struct epoll_event ev;
  ev.data.fd = cfd;
  ev.events = EPOLLIN | EPOLLET; // Edge Trigger mode
  int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
  if (ret == -1)
  {
    perror("epoll_ctl add cfd error");
    return nullptr;
  }

  return nullptr;
}

static void *recvHttpRequest(void *arg)
{
  cout << "recvHttpRequest thread id... " << this_thread::get_id() << endl;
  auto info = (struct FdInfo *)arg;

  int len = 0, total = 0;
  char tmp[1024] = {0};
  char buf[4096] = {0};
  while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
  {
    if (total + len < sizeof buf)
    {
      memcpy(buf + total, tmp, len);
    }
    total += len;
  }

  if (len == -1 and errno == EAGAIN)
  {
    parseRequestLine(buf, info->fd);
  }
  else if (len == 0)
  {
    epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, nullptr);
    close(info->fd);
  }
  else
  {
    perror("recv error");
  }
  return nullptr;
}

static int parseRequestLine(const char *line, int cfd)
{
  cout << "parseRequestLine..." << endl;
  char method[12], path[1024], protocol[12];
  sscanf(line, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol); // TODO ?
  cout << format("before decode: [{}] [{}] [{}]\n", method, path, protocol);
  decodeMsg(path, path);
  cout << format("after decode: [{}] [{}] [{}]\n", method, path, protocol);
  if (strcasecmp(method, "GET") != 0)
  {
    cout << "not get" << endl;
    return -1;
  }

  const char *file = nullptr;

  // deal the static file which client asked
  if (strcmp(path, "/") == 0)
    file = string("./").c_str();
  else
    file = path + 1;

  // get file property
  filesystem::path filePath(file);
  if (!filesystem::exists(filePath))
  {
    // File does not exist
    sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
    sendFile("404.html", cfd);
    return 0;
  }

  if (std::filesystem::is_directory(filePath))
  {
    sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
    sendDir(filePath.c_str(), cfd);
  }
  else
  {
    sendHeadMsg(cfd, 200, "OK", getFileType(filePath.extension().c_str()),
                std::filesystem::file_size(filePath));
    sendFile(filePath.c_str(), cfd);
  }
  return 0;
}

static int sendHeadMsg(int cfd, int status, const char *descr,
                       const char *type, int length)
{
  // response line
  char buf[4096] = {0};
  sprintf(buf, "HTTP/1.1 %d %s\r\n", status, descr);

  // response head
  sprintf(buf + strlen(buf), "Content-Type:%s\r\n", type);
  if (length != -1)
  {
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", length);
  }
  strcat(buf, "\r\n");
  send(cfd, buf, strlen(buf), 0);
  return 0;
}

static int sendFile(const char *fileName, int cfd)
{
  cout << "send file: " << fileName << endl;
  ifstream file(fileName, ios::binary);
  assert(file && "Failed to open file");

  file.seekg(0, ios::end);
  int size = file.tellg();
  file.seekg(0, ios::beg);

  vector<char> buffer(size);
  file.read(buffer.data(), size);

  int offset = 0;
  while (offset < size)
  {
    int ret = send(cfd, buffer.data() + offset, size - offset, 0);
    usleep(1);
    printf("ret=%d\n", ret);
    offset += ret;
    if (ret == -1)
    {
      if (errno == EAGAIN)
      {
        printf("no data.../");
      }
      perror("sendfile");
    }
  }

  file.close();
  return 0;
}

static int sendDir(const char *dirName, int cfd)
{
  cout << "send dir: " << dirName << endl;
  string buf = "<html><head><title>" + string(dirName) + "</title></head><body><table>";

  DIR *dir = opendir(dirName);
  if (dir)
  {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
      string name = entry->d_name;
      string subPath = string(dirName) + "/" + name;
      struct stat st;
      if (stat(subPath.c_str(), &st) == 0)
      {
        if (S_ISDIR(st.st_mode))
        {
          buf += "<tr><td><a href=\"" + name + "/\">" + name + "</a></td><td>" + to_string(st.st_size) + "</td></tr>";
        }
        else
        {
          buf += "<tr><td><a href=\"" + name + "\">" + name + "</a></td><td>" + to_string(st.st_size) + "</td></tr>";
        }
      }
    }
    closedir(dir);
  }

  buf += "</table></body></html>";
  send(cfd, buf.c_str(), buf.length(), 0);
  return 0;
}

static const char *getFileType(const char *name)
{
  const char *dot = strrchr(name, '.');
  // judge file type by file name
  if (dot == NULL)
    return "text/plain;charset=utf-8";
  if (strcmp(dot, ".html") == 0 || strcmp(dot, "htm") == 0)
    return "text/html;charset=utf-8";
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(dot, ".gif") == 0)
    return "image/gif";
  if (strcmp(dot, ".png") == 0)
    return "image/png";
  if (strcmp(dot, ".css") == 0)
    return "text/css";
  if (strcmp(dot, "au") == 0)
    return "audio/.au";
  if (strcmp(dot, ".wav") == 0)
    return "audio/.wav";
  if (strcmp(dot, ".avi") == 0)
    return "video/x-msvideo";
  if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    return "video/quicktime";
  if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
    return "video/mepeg";
  if (strcmp(dot, ".mp3") == 0)
    return "audio/mpeg";
  if (strcmp(dot, ".vrml") == 0)
    return "model/vrml";
  if (strcmp(dot, ".ogg") == 0)
    return "application/ogg";
  if (strcmp(dot, ".pac") == 0)
    return "application/x-nx-proxy-autoconfig";

  return "text/plain;charset=utf-8";
}

static int hexToDec(char c)
{
  if (c >= '0' and c <= '9')
    return c - '0';
  if (c >= 'a' and c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' and c <= 'F')
    return c - 'A' + 10;
  return 0;
}

static void decodeMsg(char *to, char *from)
{
  for (; *from != '\0'; ++to, ++from)
  {
    // if the char is '%', then it is a hex number
    if (from[0] == '%' and isxdigit(from[1]) and isxdigit(from[2]))
    {
      *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
      from += 2;
    }
    else
    {
      *to = *from;
    }
  }
  *to = '\0';
}