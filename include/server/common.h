#ifndef COMMON_H
# define COMMON_H

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
    int         scrHeight;
}               frame_info;

#endif
