#ifndef SERVER_H
# define SERVER_H

/* X11 Header -lX11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h> 
#include <X11/Xatom.h>

typedef struct  compression_info_t {
    int         original_size;
    int         max_compressed_size;
    char        *compressed_data;
}               compression_info;

typedef struct  stream_cmpr_info_t {
    int         block_size;
    int         max_compressed_size;
    char        *compressed_data;
}               stream_cmpr_info;

/* src_size is width * height * pixel_size */
typedef struct  frame_info_t {
    int         width;
    int         height;
    int         pixel_size;
}               frame_info;

typedef struct  xwindow_info_t {
    Display         *dis;
    Screen          *scr;    
    Drawable        drw;
    frame_info      frame_i;
}               xwindow_info;

#endif /* SERVER_H */
