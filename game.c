#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define OLIVE_IMPLEMENTATION
#include "ext/olive.c"

#include "game.h"
#include "util.h"

static inline void 
do_render(
        buffer *buf, 
        Camera cam, 
        float fps) 
{
    Olivec_Canvas oc = olivec_canvas(
            (uint32_t*)buf->mem, 
            buf->w, 
            buf->h, 
            buf->w);

    int tile_count_x = 15;
    int tile_count_z = 15;
    float tile_size = 100.0f;
    float floor_y = 100.0f;
    
    create_background(oc, 0xFF87de87);
    create_floor(
            buf, 
            oc, 
            tile_count_x, 
            tile_count_z, 
            tile_size, 
            floor_y, 
            cam);
    
    float wall_h = 30.0f;
    float wall_thick = 0.2f;
    float zig_offset = 0.5f;
    float prev_offset = 0.0f;

    for (int tx = -1; tx < 5; tx++) 
    {
        float z_base = 4 + tx * tile_size;
        float z_offset = (tx & 1) ? zig_offset : 0.0f;
        float x0 = tx * tile_size;
        float x1 = x0 + tile_size;

        float y0 = floor_y - wall_h;
        float y1 = floor_y;

        Vec3 v0 = { x0, y0, z_base + z_offset };
        Vec3 v1 = { x1, y0, z_base + z_offset };
        Vec3 v2 = { x1, y1, z_base + z_offset };
        Vec3 v3 = { x0, y1, z_base + z_offset };
        place_rect(buf, oc, v0, v1, v2, v3, 0xFF222222, cam);

        if (prev_offset != z_offset) 
        {
            Vec3 s0 = { x0, y0, z_base + prev_offset };
            Vec3 s1 = { x0, y0, z_base + z_offset };
            Vec3 s2 = { x0, y1, z_base + z_offset };
            Vec3 s3 = { x0, y1, z_base + prev_offset };
            place_rect(buf, oc, s0, s1, s2, s3, 0xFF222222, cam);
        }

        prev_offset = z_offset;
    }
 
    char text[32];
    snprintf(text, sizeof(text), "[FPS] %.1f", fps);
    place_text(oc, text);
}


int
main()
{
    Display* disp = XOpenDisplay(0);
    Window root =   XDefaultRootWindow(disp);

    int def_screen = DefaultScreen(disp);
    GC ctx =         XDefaultGC(disp, def_screen);

    int screen_depth = 24;
    XVisualInfo vis_info = {};
    if(STATUS_ERROR == XMatchVisualInfo(
                disp,
                def_screen,
                screen_depth,
                TrueColor,
                &vis_info))
    {
        printf("[ERROR] No matching visual info\n");
    }

    int win_x = 0;
    int win_y = 0;
    int win_w = 1200;
    int win_h = 800;
    int border_w = 0;
    int win_depth = vis_info.depth;
    int win_class = InputOutput;
    Visual* win_vis = vis_info.visual;

    int attr_mask = CWBackPixel | CWEventMask;
    XSetWindowAttributes win_attrs = {};
    win_attrs.event_mask =
        StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

    Window win = XCreateWindow(disp, root,
            win_x, win_y, win_w, win_h,
            border_w, win_depth, win_class, win_vis,
            attr_mask, &win_attrs);

    XMapWindow(disp, win);
    XStoreName(disp, win, "");

    Atom wm_delete = XInternAtom(disp, "WM_DELETE_WINDOW", False);
    if(!XSetWMProtocols(disp, win, &wm_delete, 1))
    {
        printf("[ERROR] Couldn't register WM_DELETE_WINDOW property \n");
    }

    int bpp = 32;
    int bytes_per_px = bpp / 8;

    buffer buf = {};
    buf.w = win_w;
    buf.h = win_h;
    buf.pitch = buf.w * bytes_per_px;
    buf.size  = buf.pitch * buf.h;
    buf.mem   = (uint8_t *)malloc(buf.size);

    Camera cam = {};
    cam.distance = 300.0f;
    cam.angle_x = 210.0f;
    cam.angle_y = 0.0f;
    cam.pos_x = -50.0f;
    cam.pos_y = 0.0f;
    cam.pos_z = 400.0f;

    KeyState keys = {};

    float triangle_angle = 0;

    int offset = 0;
    int scanline_bytes = 0;
    XImage *img = XCreateImage(disp, vis_info.visual, vis_info.depth, ZPixmap,
            offset, (char *)buf.mem, win_w, win_h, bpp, scanline_bytes);

    uint64_t end = NANO();

    float fps = 0.0f;
    float fps_update_timer = 0.0f;
    const float FPS_UPDATE_INTERVAL = 0.1f;

    int is_open = 1;
    while(is_open)
    {
        while(XPending(disp) > 0)
        {
            XEvent ev = {};
            XNextEvent(disp, &ev);

            switch(ev.type)
            {
                case KeyPress:
                {
                    XKeyPressedEvent *key_ev = (XKeyPressedEvent *)&ev;
                    KeySym keysym = XLookupKeysym(key_ev, 0);

                    if (keysym == XK_Escape) is_open = 0;

                    if (keysym == XK_w) keys.w = 1;
                    if (keysym == XK_a) keys.a = 1;
                    if (keysym == XK_s) keys.s = 1;
                    if (keysym == XK_d) keys.d = 1;
                    if (keysym == XK_Shift_L) keys.shift = 1;
                    if (keysym == XK_space) keys.space = 1;

                    if (keysym == XK_Up) keys.up = 1;
                    if (keysym == XK_Down) keys.down = 1;
                    if (keysym == XK_Left) keys.left = 1;
                    if (keysym == XK_Right) keys.right = 1;
                }
                break;

                case KeyRelease:
                {
                    XKeyPressedEvent *key_ev = (XKeyPressedEvent *)&ev;
                    KeySym keysym = XLookupKeysym(key_ev, 0);

                    if (keysym == XK_w) keys.w = 0;
                    if (keysym == XK_a) keys.a = 0;
                    if (keysym == XK_s) keys.s = 0;
                    if (keysym == XK_d) keys.d = 0;
                    if (keysym == XK_Shift_L) keys.shift = 0;
                    if (keysym == XK_space) keys.space = 0;

                    if (keysym == XK_Up) keys.up = 0;
                    if (keysym == XK_Down) keys.down = 0;
                    if (keysym == XK_Left) keys.left = 0;
                    if (keysym == XK_Right) keys.right = 0;
                }
                break;

                case ClientMessage:
                {
                    XClientMessageEvent *msg_ev = (XClientMessageEvent *) &ev;
                    if ((Atom)msg_ev->data.l[0] == wm_delete)
                    {
                        XDestroyWindow(disp, win);
                        XCloseDisplay(disp);
                        is_open = 0;
                    }
                }
                break;

                case ConfigureNotify:
                {
                    XConfigureEvent *cfg_ev = (XConfigureEvent *)&ev;
                    win_w = cfg_ev->width;
                    win_h = cfg_ev->height;

                    XDestroyImage(img);

                    buf.w = win_w;
                    buf.h = win_h;
                    buf.pitch = buf.w * bytes_per_px;
                    buf.size = buf.pitch * buf.h;
                    buf.mem = (uint8_t *)malloc(buf.size);

                    img = XCreateImage(disp, vis_info.visual, vis_info.depth, ZPixmap,
                            offset, (char *)buf.mem, win_w, win_h, bpp, scanline_bytes);
                } break;
            }
        }

        uint64_t begin = NANO();
        uint64_t delta = begin - end;
        end = begin;

        float dt = (float)delta / 1e9f;

        fps_update_timer += dt;
        if (fps_update_timer >= FPS_UPDATE_INTERVAL) 
        {
            fps = 1.0f / dt;
            fps_update_timer = 0.0f;
        }

        update_camera(&cam, &keys, dt);

        do_render(&buf, cam, fps);
        XPutImage(disp, win, ctx, img, 0, 0, 0, 0, win_w, win_h);

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = (DELTA_TIME - (NANO() - begin));
        if(ts.tv_nsec > 0) nanosleep(&ts, NULL);
    }
}
