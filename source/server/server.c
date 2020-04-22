/* Standard Header */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

/* Network Header */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Compression Header */
#include <lz4.h>

/* Local Header */
#include "server.h"

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);

    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return (-1);
    }
    return (0);
}

int init_server(int port) {
    struct sockaddr_in  serv_addr;
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (serv_sock == -1) {
        perror("socket");
        return (-1);
    }

    int t = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)) == -1) {
        perror("setsockopt");
        return (-1);
    }

    //if (set_nonblock(serv_sock) == -1)
    //    return (-1);

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        return (-1);
    }

    if (listen(serv_sock, SOMAXCONN) == -1) {
        perror("listen");
        return (-1);
    }
    return (serv_sock);
}

int frame_init(frame_info *frame, Display *dis, Screen *scr, Drawable *drw) {
    XImage *img =
        XGetImage(dis, *drw, 0, 0, scr->width, scr->height, AllPlanes, ZPixmap);

    if (img == NULL) {
        fprintf(stderr, "XGetImage: fail to retrieve screen image\n");
        return (-1);
    }

    frame->width = img->width;
    frame->height = img->height;
    frame->pixel_size = img->bits_per_pixel / 8;

    XDestroyImage(img);
    return (0);
}

int xwindow_init(xwindow_info *xwin) {
    xwin->dis = XOpenDisplay((char *)0);
    xwin->scr = XDefaultScreenOfDisplay(xwin->dis);
    xwin->drw = XDefaultRootWindow(xwin->dis);
    if (frame_init(&xwin->frame_i, xwin->dis, xwin->scr, &xwin->drw) == -1)
        return (-1);
    return (0);
}

int compression_init(compression_info *info, frame_info const *frame) {
    info->original_size = frame->width * frame->height * frame->pixel_size;
    info->max_compressed_size = LZ4_compressBound(info->original_size);
    info->compressed_data = malloc((size_t)info->max_compressed_size + 4); // M

    if (info->compressed_data == NULL) {
        perror("malloc");
        return (-1);
    }
    return (0);
}

int process_img(xwindow_info *xwin, compression_info *cmpr) {
    XImage *img = XGetImage(xwin->dis, xwin->drw, 0, 0, xwin->scr->width,
                            xwin->scr->height, AllPlanes, ZPixmap);

    if (img == NULL) {
        fprintf(stderr, "XGetImage: fail to retrieve screen image\n");
        return (-1);
    }

    int cmpr_size = LZ4_compress_default(img->data, cmpr->compressed_data + 4, // M
                                         cmpr->original_size,
                                         cmpr->max_compressed_size);

    // debug
    if (cmpr_size > 0) {
        printf("We successfully compressed some data! Ratio: %.2f\n",
            (float) cmpr_size / cmpr->original_size);
    }
    printf("%d\n", cmpr_size);

    XDestroyImage(img);
    return (cmpr_size);
}

int connection_init(int sock, frame_info *frame_i) {
    write(sock, frame_i, sizeof(frame_i));
    return (0);
}

int send_cmpr_img(int sock, int cmpr_size, char *cmpr_data) {
    int *data = (int *)cmpr_data;

    data[0] = cmpr_size;
    write(sock, cmpr_data, cmpr_size + 4);
    return (0);
}

int parse_cmd_line(int ac, char **av, int *port) {
    if (ac != 2 || atoi(av[1]) == 0) {
        fprintf(stderr, "usage: %s <port>\n", av[0]);
        return (-1);
    }
    *port = atoi(av[1]);
    return (0);
}

int main(int ac, char **av) {
    int serv_sock;
    int port;

    if (parse_cmd_line(ac, av, &port) == -1)
        return (-1);

    serv_sock = init_server(port);
    if (serv_sock == -1)
        return (errno);

    int                 cli_sock;
    struct sockaddr_in  cli_addr;
    socklen_t           addr_len = sizeof(cli_addr);

    cli_sock = accept(serv_sock, (struct sockaddr *)&cli_addr, &addr_len);
    if (cli_sock == -1) {
        perror("accept");
        return (errno);
    }

    xwindow_info        xwin_i;
    compression_info    compression_i;
    int                 cmpr_size = 0;

    if (xwindow_init(&xwin_i) == -1 ||
        compression_init(&compression_i, &xwin_i.frame_i) == -1)
        return (-1);

    connection_init(cli_sock, &xwin_i.frame_i);
    while (1) {
        cmpr_size = process_img(&xwin_i, &compression_i);
        send_cmpr_img(cli_sock, cmpr_size, compression_i.compressed_data);
    }

    XCloseDisplay(xwin_i.dis);
    close(cli_sock);
    close(serv_sock);
    return (0);
}
