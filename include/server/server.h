#ifndef SERVER_H
# define SERVER_H

typedef struct  compression_info_t {
    int         original_size;
    int         max_compressed_size;
    char        *compressed_data;
}               compression_info;

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
