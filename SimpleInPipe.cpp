//Nabin Rijal, 2015

// C++ includes
#include <cstdlib>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>

// project includes
#include "SimpleInPipe.h"

// making STD available
using namespace std;

bool SimpleInPipe::open(char* argv[])
{
  if(isopen)
    ::close(fd);

  eof_flag = false;

  // open the pipe
  int fds[2];
  if(pipe(fds))
  {
    cerr << "unable to create pipe" << endl;
    return false;
  }

  // fork off the subprocess
  pid_t pid = fork();
  if(pid < 0)
  {
    cerr << "unable to fork()" << endl;
    return false;
  }

  if(pid == 0)
  {
    ::close(fds[0]);
    int out(fileno(stdout));
    ::close(out);
    dup2(fds[1], out);
    ::close(fds[1]);
    execvp(argv[0],argv);
    cerr << "failed to execute child process " << argv[0] << endl;
    exit(errno);
  }
  else
  {
    ::close(fds[1]);
    fd = fds[0];
    isopen = true;
    return true;
  }
}

int SimpleInPipe::read(char* buffer, unsigned n)
{
  if(!isopen) return 0;

  unsigned total(0);
  while(total != n)
  {
    unsigned actual(::read(fd, buffer, n - total));
    if(actual > 0)
    {
      total += actual;
      buffer += actual;
    }
    else
    {
      eof_flag = true;
      break;
    }
  }
  return total;
}

