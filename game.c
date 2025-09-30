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
        Light light,
        Level *level,
        Camera cam, 
        float fps) 
{
    Olivec_Canvas oc = olivec_canvas(
            (uint32_t*)buf->mem, 
            buf->w, 
            buf->h, 
            buf->w);

    for (int i = 0; i < buf->w * buf->h; i++) 
        buf->depth_buffer[i] = 1.0f;
    
    create_background(oc);
    create_floor(
            buf, 
            oc, 
            15, 
            15, 
            100, 
            100, 
            light, cam);
    
    level_render(level, buf, oc, light, cam);

    char text[16];
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
    buf.shadow_w = 256;
    buf.shadow_h = 256;
    buf.pitch = buf.w * bytes_per_px;
    buf.size  = buf.pitch * buf.h;
    buf.mem   = (uint8_t *)malloc(buf.size);
    buf.depth_buffer = (float *)malloc(sizeof(float) * buf.w * buf.h);
    buf.shadow_depth = malloc(sizeof(float) * buf.shadow_w * buf.shadow_h);
    
    for (int i = 0; i < buf.shadow_w * buf.shadow_h; i++)
        buf.shadow_depth[i] = 1e9f;

    Camera cam = {};
    cam.distance = 300.0f;
    cam.angle_x = 210.0f;
    cam.angle_y = 0.0f;
    cam.pos_x = -50.0f;
    cam.pos_y = 0.0f;
    cam.pos_z = 400.0f;

    Light sun = {};
    sun.position = (Vec3){0, 0, 0};
    sun.direction = (Vec3){0.3f, 1.0f, 0.3f};
    sun.color = 0xFFFFFFFF;
    sun.intensity = 0.2f;
    sun.is_directional = 1;

    KeyState keys = {};

    float triangle_angle = 0;

    int offset = 0;
    int scanline_bytes = 0;
    XImage *img = XCreateImage(disp, vis_info.visual, vis_info.depth, ZPixmap,
            offset, (char *)buf.mem, win_w, win_h, bpp, scanline_bytes);

    Level* level = level_load_from_file("level.txt");
    if (!level)
    {
        printf("[ERROR] Failed to load level, exiting\n"); 
        return 1;
    }

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
                        level_free(level);
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
                    free(buf.depth_buffer);
                    buf.depth_buffer = 
                        (float *)malloc(sizeof(float) * buf.w * buf.h);

                    buf.w = win_w;
                    buf.h = win_h;
                    buf.pitch = buf.w * bytes_per_px;
                    buf.size = buf.pitch * buf.h;
                    buf.mem = (uint8_t *)malloc(buf.size);

                    img = XCreateImage(
                            disp, 
                            vis_info.visual, 
                            vis_info.depth, 
                            ZPixmap,
                            offset, 
                            (char *)buf.mem, 
                            win_w, win_h, 
                            bpp, 
                            scanline_bytes);
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

        do_render(&buf, sun, level, cam, fps);
        XPutImage(disp, win, ctx, img, 0, 0, 0, 0, win_w, win_h);

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = (DELTA_TIME - (NANO() - begin));
        if(ts.tv_nsec > 0) nanosleep(&ts, NULL);
    }
}
