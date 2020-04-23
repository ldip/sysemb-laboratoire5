#include "client/client.h"
#include "server/common.h"

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

void put_pixel(char *fbp, int lineLength, int page_size, int cur_page, int x, int y, uint32_t c)
{
    // calculate the pixel's byte offset inside the buffer
    unsigned int pix_offset = x + y * lineLength;

    // offset by the current buffer start
    pix_offset += cur_page * page_size;

    // now this is about the same as 'fbp[pix_offset] = value'
		//memcpy(fbp + pix_offset, &c, sizeof(uint32_t));
    *((char*)(fbp + pix_offset)) = c;

}

// helper function to draw a rectangle in given color
void fill_rect(char *fbp, int lineLength, int page_size, int cur_page, int x, int y, int w, int h, uint32_t c) {
    int cx, cy;
    for (cy = 0; cy < h; cy++) {
        for (cx = 0; cx < w; cx++) {
            put_pixel(fbp, lineLength, page_size, cur_page, x + cx, y + cy, c);
        }
    }
}

void display(framebuffer_info *fb, char *image, frame_info *frameInfo)
{
	static int currentPage = 0;
	currentPage = (currentPage + 1) % 2;
	uint32_t tmp;

	/*printf("OK %d\n", fb->info->yres);
	int w = fb->info->yres / 10;
  int h = w;

	printf("LOL\n");
	fill_rect(fb->data, fb->lineLength, fb->lineLength * fb->info->yres, currentPage, 0, 0, w, h, 240);*/

	/*int w = fb->info->yres / 10;
  int h = w;*/

	/*printf("LOL\n");
	for (int cy = 0 ; cy < fb->info->yres ; cy++) {
		for (int cx = 0 ; cx < fb->info->xres * 4 ; cx++) {
			//memcpy(&tmp, data + cx * cy, sizeof(uint32_t));
			put_pixel(fb->data, fb->lineLength, fb->lineLength * fb->info->yres, currentPage, cx, cy, 12);
		}
	}*/

	//printf("Display size %d\n", fb->lineLength * fb->info->yres);
	/*uint32_t *data = malloc(frameInfo->width * frameInfo->height * frameInfo->pixel_size);
	int fd = open("screenshot2", O_RDWR);
	read(fd, data, frameInfo->width * frameInfo->height * frameInfo->pixel_size);*/
	memcpy(fb->data + currentPage * fb->lineLength * fb->info->yres, image, fb->lineLength * fb->info->yres);

	/*unsigned char *currentFramebuffer = fb->data + currentPage * fb->lineLength * fb->info->yres;

	for (int y = 0 ; y < fb->info->yres ; y++) {
		memcpy(currentFramebuffer + y * fb->lineLength, data + y * frameInfo->width, fb->info->xres);
	}*/

	/*for (int y = 0 ; y < fb->info->yres ; y++) {
		for (int x = 0 ; x < fb->info->xres ; x++) {
			memcpy(currentFramebuffer + x + y * fb->lineLength, &tmp, 32);
			//*((char*)(currentFramebuffer + x + y * fb->lineLength)) = 124;
		}
	}*/

	/*for (unsigned int ligne = 0 ; ligne < frameInfo->height ; ligne++){
		memcpy(currentFramebuffer + ligne * fb->lineLength, data + ligne * frameInfo->width, fb->info->xres);
	}*/

	printf("COOL %d\n", fb->lineLength);

	fb->info->xoffset = 0;
	fb->info->yoffset = currentPage * fb->info->yres;
	fb->info->activate = FB_ACTIVATE_VBL;
	if (ioctl(fb->fd, FBIOPAN_DISPLAY, fb->info)) {
		printf("Erreur lors du changement de buffer (double buffering inactif)!\n");
	}
	printf("Ca sort\n");
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

	//fdInfo->nonstd = 0;
	//fdInfo->colorspace = V4L2_PIX_FMT_RGB32;

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

/*compression_info *getCompInfo(frame_info *frameInfo)
{
	compression_info *compInfo = malloc(sizeof(compression_info));

	compInfo->original_size = frameInfo->width * frameInfo->height * frameInfo->pixel_size;
	compInfo->max_compressed_size = LZ4_compressBound(compInfo->original_size);
	compInfo->compressed_data = malloc(compInfo->max_compressed_size);
	return compInfo;
}*/

stream_cmpr_info *getCompInfo(frame_info *frameInfo)
{
	stream_cmpr_info *compInfo = malloc(sizeof(stream_cmpr_info));
	compInfo->block_size = frameInfo->width * frameInfo->pixel_size;
  compInfo->max_compressed_size = LZ4_COMPRESSBOUND(compInfo->block_size);
  compInfo->compressed_data = malloc((size_t)compInfo->max_compressed_size);

  if (compInfo->compressed_data == NULL) {
		perror("malloc");
    return NULL;
  }
  return compInfo;
}

/*int readData(int sock, compression_info *compInfo)
{
	int size_read = 0;
	int size;
	int tmp;

	read(sock, &size, 4);
  while (size_read < size) {
		printf("Ca read\n");
		tmp = read(sock, compInfo->compressed_data + size_read, size);
		printf("Ca read plus %d\n", tmp);
		if (tmp == -1)
			break;
		size_read += tmp;
	}
	write(sock, "1", 1);
	return size;
}*/

void readData(int sock, stream_cmpr_info *compInfo, char *image, int srcHeight)
{
	/*LZ4_stream_t lz4_stream;
	char *line_buf[2];
	int line_buf_idx = 0;
	int cmpr_size;

	LZ4_initStream(&lz4_stream, sizeof(LZ4_stream_t));

	for (int y = 0; y < xwin->scr->height; ++y) {
		line_buf[line_buf_idx] = img->data + (cmpr->block_size * y);
	}*/


	LZ4_streamDecode_t lz4StreamDecode_body;
  LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

  char *decPtr[2];
  int  decBufIndex = 0;
	int size;
	int sizeRead;
	int tmp;

  LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);
	for (int y = 0 ; y < srcHeight ; ++y) {
		read(sock, &size, 4);
		sizeRead = 0;

		while (sizeRead < size) {
			printf("LOL\n");
			tmp = read(sock, compInfo->compressed_data + sizeRead, size - sizeRead);
			printf("LOL2 tmp:%d sizeRead:%d size:%d request:%d/%d\n", tmp, sizeRead, size, y, srcHeight);
			if (tmp == -1)
				break;
			sizeRead += tmp;
		}

		if (sizeRead == 0)
			break;

		decPtr[decBufIndex] = image + (compInfo->block_size * y);

		const int decBytes = LZ4_decompress_safe_continue(
                lz4StreamDecode, compInfo->compressed_data, decPtr[decBufIndex], size, compInfo->block_size);

		decBufIndex = (decBufIndex + 1) % 2;
	}
	write(sock, "1", 1);
	printf("Une frame recu\n");
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

	if (argc != 3 || (port = atoi(argv[2])) <= 0) {
		printf("usage: %s <ip> <port>\n", argv[0]);
	}

	sock = getConnection(argv[1], port);
	frameInfo = getFrameInfo(sock);
	compInfo = getCompInfo(frameInfo);
	image = malloc(frameInfo->width * frameInfo->height * frameInfo->pixel_size);
	fb = initFramebuffer(frameInfo);

	/*printf("lol\n");
	compressedSize = readData(sock, compInfo);
	printf("lol2 %d\n", compInfo->original_size);
	LZ4_decompress_safe(compInfo->compressed_data, (char *) image, compressedSize, compInfo->original_size);
	printf("lol3\n");
	printf("Size %d\n", compInfo->original_size);
	display(fb, image, frameInfo);*/
	//readData(sock, compInfo, image);
	//display(fb, image, frameInfo);
	while (1) {
		readData(sock, compInfo, image, frameInfo->scrHeight);
		//printf("Ca decompresse\n");
		//LZ4_decompress_safe(compInfo->compressed_data, (char *) image, compressedSize, compInfo->original_size);
		//printf("Ca decompresse plus\n");
		display(fb, image, frameInfo);
	}
  return 0;
}
