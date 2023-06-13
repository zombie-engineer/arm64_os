#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  int fd;
  int n;
  char buf[4] = { 0x11, 0x33, 0x11, 0xdd };
  struct termios options;
  buf[0] = 'e';
  buf[1] = 'r';
  buf[2] = '1';
  buf[3] = '2';
  fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    perror("failed to open ttyUSB");
    return -1;
  }
  n = tcgetattr(fd, &options);
  if (n == -1) {
    perror("failed to get tty options");
    return -1;
  }

  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_cflag |= CRTSCTS;
  tcsetattr(fd, TCSANOW, &options);
  ioctl(fd, TCFLSH, 2);
  
  // buf[0] = 'x';
  n = write(fd, buf, 1);
  n = -1;
  memset(buf, 0, sizeof(buf));
  while(n == -1) {
     n = read(fd, buf, 2);
  }
  return 0;
}
