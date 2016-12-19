#ifndef SIMPLEINPIPE_H
#define SIMPLEINPIPE_H

// C includes
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

class SimpleInPipe {
  int fd;
  bool isopen;
  bool eof_flag;
public:
  SimpleInPipe() : isopen(false), eof_flag(false) {}
  SimpleInPipe(char* argv[]) { open(argv); }
  ~SimpleInPipe() { if(isopen) ::close(fd); }
  bool open(char* argv[]);
  int read(char* buffer, unsigned n);
  bool eof() { return eof_flag; }
  void close() { ::close(fd); isopen = false; }
};

#endif

