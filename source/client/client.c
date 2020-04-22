#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/fb.h>

#include <lz4.h>

#include "server.h"

int getConnection(char *addr, int port)
{
	int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

	return sock;
}

unsigned char *initDisplay()
{
	struct fb_var_screeninfo fdInfo;
	struct fb_fix_screeninfo finfo;
	uint32_t screensize;
	int fbfd = open("/dev/fb0", O_RDWR);

	ioctl(fbfd, FBIOGET_VSCREENINFO, &fdInfo);

	fdInfo.bits_per_pixel = 32;
	fdInfo.xres = 1920;
	fdInfo.yres = 1080;

	ioctl(fbfd, FBIOPUT_VSCREENINFO, &fdInfo);

	ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);

	screensize = finfo.smem_len;

	return mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
}

frame_info *getFrameInfo(int sock)
{
	frame_info *frameInfo = malloc(sizeof(frame_info));

	read(sock, frameInfo, sizeof(frameInfo));
	return frameInfo;
}

compression_info *getCompInfo(frame_info *frameInfo)
{
	compression_info *compInfo = malloc(sizeof(compression_info));

	compInfo->original_size = frameInfo->width * frameInfo->height * frameInfo->pixel_size;
	compInfo->max_compressed_size = LZ4_compressBound(compInfo->original_size);
	compInfo->compressed_data = malloc(compInfo->max_compressed_size);
	return compInfo;
}

int readData(int sock, compression_info *compInfo)
{
	int size;

	read(sock, &size, sizeof(int));
	read(sock, compInfo->compressed_data, size);
	return size;
}

int main(int argc, char *argv[])
{
	int port;
	int sock;
	unsigned char *screen;
	frame_info *frameInfo;
	compression_info *compInfo;
	int compressedSize;
	char *image;

	if (argc != 3 || (port = atoi(argv[2])) <= 0) {
		printf("usage: %s <ip> <port>\n", argv[0]);
	}

	sock = getConnection(argv[1], port);
	screen = initDisplay();
	frameInfo = getFrameInfo(sock);
	compInfo = getCompInfo(frameInfo);
	image = malloc(compInfo->original_size);

	while (1) {
		compressedSize = readData(sock, compInfo);
		LZ4_decompress_safe(compInfo->compressed_data, image, compressedSize, compInfo->original_size);
	}
    return 0;
}
