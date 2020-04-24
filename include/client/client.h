#ifndef CLIENT_H
# define CLIENT_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/fb.h>
#include <linux/videodev2.h>

#include <lz4.h>

typedef struct framebuffer_info {
  char *data;
  int fd;
  struct fb_var_screeninfo *info;
  int lineLength;
} framebuffer_info;

#endif
