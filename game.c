#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define OLIVE_IMPLEMENTATION
#include "../ext/olive.c"

#include "game.h"
#include "util.h"

static inline void do_editor(buffer *buf, Level *level, Camera cam)
{
    Olivec_Canvas oc = olivec_canvas((uint32_t*)buf->mem, buf->w, buf->h, buf->w);
    create_background(oc, 0xFF0B0B0B);  

    float minx = 1e9f, maxx = -1e9f, minz = 1e9f, maxz = -1e9f;
    for (int i = 0; i < level->wall_count; ++i) 
    {
        Wall *w = &level->walls[i];
        if (w->type != FLOOR) 
        {
            float a = w->angle;
            float dx = (w->type == WALL_Z) ? w->width : 0.0f;
            float dz = (w->type == WALL_X) ? w->width : 0.0f;
            float rx = dx * cosf(a) - dz * sinf(a);
            float rz = dx * sinf(a) + dz * cosf(a);
            float x0 = w->pos.x,          z0 = w->pos.z;
            float x1 = w->pos.x + rx,     z1 = w->pos.z + rz;
            if (x0 < minx) minx = x0; if (x0 > maxx) maxx = x0;
            if (z0 < minz) minz = z0; if (z0 > maxz) maxz = z0;
            if (x1 < minx) minx = x1; if (x1 > maxx) maxx = x1;
            if (z1 < minz) minz = z1; if (z1 > maxz) maxz = z1;
        } 
        else {}
    } 

    const float margin = 40.0f;
    float sx = (buf->w - 2.0f*margin) / fmaxf(1e-3f, (maxx - minx));
    float sy = (buf->h - 2.0f*margin) / fmaxf(1e-3f, (maxz - minz));
    float scale = fminf(sx, sy);
    #define SXX(X) (int)(margin + ((X) - minx) * scale)
    #define SYY(Z) (int)(margin + ((Z) - minz) * scale)

    int mx = -10000, my = -10000;
    {
        Display *d = XOpenDisplay(NULL);
        if (d) 
        {
            Window rr, cr;
            int rx, ry, wx, wy; unsigned int mask;
            if (XQueryPointer(d, DefaultRootWindow(d), &rr, &cr, &rx, &ry, &wx, &wy, &mask)) {
                mx = rx; my = ry;
            }
            XCloseDisplay(d);
        }
    }


    const int HR = 3;
    float best = 1e9f; 
    int hx0=0, hy0=0, hx1=0, hy1=0;

    for (int i = 0; i < level->wall_count; ++i) 
    {
        Wall *w = &level->walls[i];

        if (w->type != FLOOR) 
        {
            float a = w->angle;
            float dx = (w->type == WALL_Z) ? w->width : 0.0f;
            float dz = (w->type == WALL_X) ? w->width : 0.0f;
            float rx = dx * cosf(a) - dz * sinf(a);
            float rz = dx * sinf(a) + dz * cosf(a);
            int x0 = SXX(w->pos.x);
            int y0 = SYY(w->pos.z);
            int x1 = SXX(w->pos.x + rx);
            int y1 = SYY(w->pos.z + rz);

            olivec_line(oc, x0, y0, x1, y1, 0xFFCC4A4A);
            olivec_rect(oc, x0 - HR, y0 - HR, 2*HR+1, 2*HR+1, 0xFF3B82F6);
            olivec_rect(oc, x1 - HR, y1 - HR, 2*HR+1, 2*HR+1, 0xFF3B82F6);

            float vx = (float)(x1 - x0), vy = (float)(y1 - y0);
            float wxv = (float)(mx - x0), wyv = (float)(my - y0);
            float vv = vx*vx + vy*vy;
            float t = vv > 1e-6f ? (vx*wxv + vy*wyv) / vv : 0.0f;
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
            float cx = x0 + t*vx, cy = y0 + t*vy;
            float dx2 = mx - cx, dy2 = my - cy;
            float d = sqrtf(dx2*dx2 + dy2*dy2);
            if (d < best) { best = d; hx0=x0; hy0=y0; hx1=x1; hy1=y1; }
        }
        else {}
    }

    if (best < 50.0f) olivec_line(oc, hx0, hy0, hx1, hy1, 0xFFEAD14B);

    #undef SXX
    #undef SYY
}

static inline void do_render(
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
    
    create_background(oc, g_fog_color);
    create_floor(
            buf, 
            oc, 
            100, 
            100, 
            100, 
            100,
            0xFFaaefbb,
            0xFF78de99, // 0xFFaaddff,
            light, cam);
    
    level_render(level, buf, oc, light, cam);
    char text[32];
    snprintf(text, sizeof(text), "[fps]%.1f", fps);
    place_text(oc, text);
}

int main()
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
        StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

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
    buf.depth_buffer = (float *)malloc(sizeof(float) * buf.w * buf.h);
    
    Camera cam = {};
    cam.distance = 300.0f;
    cam.angle_x = 210.0f;
    cam.angle_y = 0.0f;
    cam.pos_x = -50.0f;
    cam.pos_y = 0.0f;
    cam.pos_z = 400.0f;

    Light sun = {};
    sun.position = (Vec3){300, -100, 200};
    sun.direction = (Vec3){0.3f, 1.0f, 0.5f};
    sun.color = 0xFFFFFFFF;
    sun.intensity = 1.0f;
    sun.is_directional = 0;

    KeyState keys = {};

    float triangle_angle = 0;

    int offset = 0;
    int scanline_bytes = 0;
    XImage *img = 
        XCreateImage(disp, vis_info.visual, vis_info.depth, 
                ZPixmap, offset, (char *)buf.mem, 
                win_w, win_h, bpp, scanline_bytes);

    Level* level = level_load_from_file("level.txt"); 

    uint64_t end = NANO();

    float fps = 0.0f;
    float fps_update_timer = 0.0f;
    const float FPS_UPDATE_INTERVAL = 0.1f;
    
    int is_open = 1;
    int editor  = 0; 
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
                    if (keysym == XK_F1) editor = editor == 1 ? 0 : 1;
                    // if (keysym == XK_s) level_save_to_file(level, "level.txt");

                    if (!editor) {
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
                }
                break;

                case KeyRelease:
                {
                    XKeyPressedEvent *key_ev = (XKeyPressedEvent *)&ev;
                    KeySym keysym = XLookupKeysym(key_ev, 0);
                    if (!editor) {
                        if (keysym == XK_w) keys.w = 0;
                        if (keysym == XK_a) keys.a = 0;
                        if (keysym == XK_s) keys.s = 0;
                        if (keysym == XK_d) keys.d = 0;
                        if (keysym == XK_Shift_L) keys.shift = 0;
                        if (keysym == XK_space) keys.space = 0;

                        if (keysym == XK_Up)    keys.up = 0;
                        if (keysym == XK_Down)  keys.down = 0;
                        if (keysym == XK_Left)  keys.left = 0;
                        if (keysym == XK_Right) keys.right = 0;
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

        if (!editor) do_render(&buf, sun, level, cam, fps);
        if ( editor) do_editor(&buf, level, cam);

        XPutImage(disp, win, ctx, img, 0, 0, 0, 0, win_w, win_h);
    }
}
