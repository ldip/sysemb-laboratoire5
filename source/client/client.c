#include "client/client.h"
#include "server/common.h"

#include <sys/time.h>

double get_time()
{
	struct timeval t;
	struct timezone tzp;
	gettimeofday(&t, &tzp);
	return (double)t.tv_sec + (double)(t.tv_usec)*1e-6;
}

double get_time_micro()
{
	struct timeval  tv;
	gettimeofday(&tv, NULL);

  return tv.tv_usec;
}

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

void display(framebuffer_info *fb, char *image, frame_info *frameInfo, double timeServer)
{
	static int currentPage = 0;
	static double time = 0;
	static double totalTime = 0;
	static int frameNbr = 0;
	currentPage = (currentPage + 1) % 2;

	memcpy(fb->data + currentPage * fb->lineLength * fb->info->yres, image, fb->lineLength * fb->info->yres);

	fb->info->xoffset = 0;
	fb->info->yoffset = currentPage * fb->info->yres;
	fb->info->activate = FB_ACTIVATE_VBL;
	if (ioctl(fb->fd, FBIOPAN_DISPLAY, fb->info)) {
		printf("Erreur lors du changement de buffer (double buffering inactif)!\n");
	}

	double now = get_time();
	if (time == 0)
		time = now;
	printf("Timing %lf Total %lf FPS %lf\n", now - time, totalTime, frameNbr / totalTime);
	totalTime += now - time;
	time = now;
	frameNbr++;

	double nowMicro = get_time_micro();
	double latency = (nowMicro - timeServer) / 1000;
	printf("Time server %lf Now %lf Latency %lf ms\n", timeServer, nowMicro, latency);
}

framebuffer_info *initFramebuffer(frame_info *frameInfo)
{
	struct fb_var_screeninfo *fdInfo = malloc(sizeof(struct fb_var_screeninfo));
	struct fb_fix_screeninfo finfo;
	uint32_t screensize;
	int fbfd = open("/dev/fb0", O_RDWR);
	framebuffer_info *fb = malloc(sizeof(framebuffer_info));

	ioctl(fbfd, FBIOGET_VSCREENINFO, fdInfo);

	fdInfo->bits_per_pixel = frameInfo->pixel_size * 8;
	fdInfo->xres = frameInfo->width;
	fdInfo->yres = frameInfo->height;

	fdInfo->xres_virtual = fdInfo->xres;
	fdInfo->yres_virtual = fdInfo->yres * 2;

	ioctl(fbfd, FBIOPUT_VSCREENINFO, fdInfo);

	ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);

	screensize = finfo.smem_len;

	fb->data = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	fb->fd = fbfd;
	fb->info = fdInfo;
	fb->lineLength = finfo.line_length;
	return fb;
}

frame_info *getFrameInfo(int sock)
{
	frame_info *frameInfo = malloc(sizeof(frame_info));

	read(sock, frameInfo, sizeof(frame_info));
	return frameInfo;
}

stream_cmpr_info *getCompInfo(frame_info *frameInfo)
{
	stream_cmpr_info *compInfo = malloc(sizeof(stream_cmpr_info));
	compInfo->block_size = frameInfo->width * frameInfo->pixel_size * 90;
  compInfo->max_compressed_size = LZ4_COMPRESSBOUND(compInfo->block_size);
  compInfo->compressed_data = malloc((size_t)compInfo->max_compressed_size);

  if (compInfo->compressed_data == NULL) {
		perror("malloc");
    return NULL;
  }
  return compInfo;
}

double readData(int sock, stream_cmpr_info *compInfo, char *image, int srcHeight)
{
	LZ4_streamDecode_t lz4StreamDecode_body;
  LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

  char *decPtr[2];
  int  decBufIndex = 0;
	int size;
	int sizeRead;
	int tmp;
	double timeServer;

  LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);

	read(sock, &timeServer, sizeof(double));

	for (int y = 0 ; y < srcHeight / 90 ; ++y) {
		sizeRead = 0;
		while (sizeRead < 4) {
			tmp = read(sock, &size + sizeRead, 4 - sizeRead);
			if (tmp != -1)
				sizeRead += tmp;
		}

		sizeRead = 0;
		while (sizeRead < size) {
			tmp = read(sock, compInfo->compressed_data + sizeRead, size - sizeRead);
			if (tmp != -1)
				sizeRead += tmp;
		}

		decPtr[decBufIndex] = image + (compInfo->block_size * y);

		const int decBytes = LZ4_decompress_safe_continue(
                lz4StreamDecode, compInfo->compressed_data, decPtr[decBufIndex], size, compInfo->block_size);

		decBufIndex = (decBufIndex + 1) % 2;
	}
	write(sock, "1", 1);

	return timeServer;
}

int main(int argc, char *argv[])
{
	int port;
	int sock;
	framebuffer_info *fb;
	frame_info *frameInfo;
	stream_cmpr_info *compInfo;
	int compressedSize;
	char *image;
	double timeServer;

	if (argc != 3 || (port = atoi(argv[2])) <= 0) {
		printf("usage: %s <ip> <port>\n", argv[0]);
	}

	sock = getConnection(argv[1], port);
	frameInfo = getFrameInfo(sock);
	compInfo = getCompInfo(frameInfo);
	image = malloc(frameInfo->width * frameInfo->height * frameInfo->pixel_size);
	fb = initFramebuffer(frameInfo);

	while (1) {
		timeServer = readData(sock, compInfo, image, frameInfo->scrHeight);
		display(fb, image, frameInfo, timeServer);
	}
  return 0;
}
