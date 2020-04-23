#ifndef SERVER_H
# define SERVER_H

/* X11 Header -lX11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h> 
#include <X11/Xatom.h>

/* Local Header */
#include "common.h"

typedef struct  xwindow_info_t {
    Display         *dis;
    Screen          *scr;    
    Drawable        drw;
    frame_info      frame_i;
}               xwindow_info;

#endif /* SERVER_H */
