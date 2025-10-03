#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OLIVE_IMPLEMENTATION
#include "../ext/olive.c"

#include "game.h"
#include "util.h"
#include "editor.h"

static inline void do_editor(buffer *buf, Level *level, Camera cam, EditorState* es, int mouse_x, int mouse_y)
{
    Olivec_Canvas oc = olivec_canvas((uint32_t*)buf->mem, buf->w, buf->h, buf->w);
    create_background(oc, 0xFF0B0B0B);

    const int HR = 3;
    float best_px = 1e3f; es->hovered_wall = -1;

    // Draw walls and compute hover
    for (int i = 0; i < level->wall_count; ++i)
    {
        Wall *w = &level->walls[i]; if (w->type == FLOOR) continue;
        float x0,z0,x1,z1; wall_endpoints(w,&x0,&z0,&x1,&z1);
        int sx0,sy0,sx1,sy1; map_to_screen(es,x0,z0,&sx0,&sy0); map_to_screen(es,x1,z1,&sx1,&sy1);
        olivec_line(oc, sx0, sy0, sx1, sy1, 0xFFCC4A4A);
        olivec_rect(oc, sx0 - HR, sy0 - HR, 2*HR+1, 2*HR+1, 0xFF3B82F6);
        olivec_rect(oc, sx1 - HR, sy1 - HR, 2*HR+1, 2*HR+1, 0xFF3B82F6);
        float dpx = seg_dist_px(es, mouse_x, mouse_y, x0,z0,x1,z1);
        if (dpx < best_px){ best_px = dpx; es->hovered_wall = i; }
    }

    // Hover highlight
    if (es->hovered_wall >= 0 && best_px < 50.0f){
        float x0,z0,x1,z1; wall_endpoints(&level->walls[es->hovered_wall],&x0,&z0,&x1,&z1);
        int sx0,sy0,sx1,sy1; map_to_screen(es,x0,z0,&sx0,&sy0); map_to_screen(es,x1,z1,&sx1,&sy1);
        olivec_line(oc, sx0, sy0, sx1, sy1, 0xFFEAD14B);
    }

    // NewWall preview
    if (es->mode == EMode_NewWall && es->started){
        float mxw,mzw; screen_to_map(es, mouse_x, mouse_y, &mxw, &mzw);
        int sxs,sys,sxe,sye;
        map_to_screen(es, es->start_x, es->start_z, &sxs, &sys);
        map_to_screen(es, mxw, mzw, &sxe, &sye);
        olivec_line(oc, sxs, sys, sxe, sye, 0xFFFFFFFF);
    }

    // HUD tip (optional)
    char text[2028];
    snprintf(text, sizeof(text), "o p zoom, arrows pan, w new wall, plus - increase height, shift plus - move pos");
    place_text(oc, text);
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

    for (int i = 0; i < (int)(buf->w * buf->h); i++)
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
        0xFF78de99,
        light, cam);

    level_render(level, buf, oc, light, cam);

    char text[32];
    snprintf(text, sizeof(text), "[fps]%.1f", fps);
    place_text(oc, text);
}

int main()
{
    Display* disp = XOpenDisplay(0);
    Window root = XDefaultRootWindow(disp);
    int def_screen = DefaultScreen(disp);
    GC ctx = XDefaultGC(disp, def_screen);
    int screen_depth = 24;

    XVisualInfo vis_info = (XVisualInfo){0};
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
    XSetWindowAttributes win_attrs = (XSetWindowAttributes){0};
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
    buf.size = buf.pitch * buf.h;
    buf.mem = (uint8_t *)malloc(buf.size);
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

    XImage *img =
        XCreateImage(disp, vis_info.visual, vis_info.depth,
            ZPixmap, 0, (char *)buf.mem,
            win_w, win_h, bpp, 0);

    Level* level = level_load_from_file("level.txt");

    EditorState es = {0};
    editor_fit_level(&es, &buf, level);

    uint64_t end = NANO();
    float fps = 0.0f;
    float fps_update_timer = 0.0f;
    const float FPS_UPDATE_INTERVAL = 0.1f;

    int is_open = 1;
    int editor = 0;
    int mouse_x = 0, mouse_y = 0;
    int dragging_active = 0; // for relative motion tracking in DragWall

    while(is_open)
    {
        while(XPending(disp) > 0)
        {
            XEvent ev = (XEvent){0};
            XNextEvent(disp, &ev);
            switch(ev.type)
            {
                case ClientMessage:
                {
                    if ((Atom)ev.xclient.data.l[0] == wm_delete) is_open = 0;
                } break;

                case KeyPress:
                {
                    XKeyPressedEvent *key_ev = (XKeyPressedEvent *)&ev;
                    KeySym keysym = XLookupKeysym(key_ev, 0);
                    if (keysym == XK_F1) editor = editor ? 0 : 1;
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
                    } else {
                        float pan_step = 200.0f / es.scale;
                        if (keysym == XK_Up)    es.pan_z -= pan_step;
                        if (keysym == XK_Down)  es.pan_z += pan_step;
                        if (keysym == XK_Left)  es.pan_x -= pan_step;
                        if (keysym == XK_Right) es.pan_x += pan_step;
                        if (keysym == XK_o) es.scale = fminf(es.scale * 1.5f, 4096.0f);
                        if (keysym == XK_p)  es.scale = fmaxf(es.scale / 1.5f, 0.01f);
                        if (keysym == XK_w) { es.mode = EMode_NewWall; es.started = 0; }
                        if (keysym == XK_Escape) 
                        { 
                            es.mode = EMode_Default; 
                            es.started = 0; 
                            es.drag_wall=-1; 
                            es.drag_endpoint=-1; 
                            dragging_active=0; 
                        }

                        // Find target wall: prefer drag wall, else hovered wall
                        int target = (es.drag_wall >= 0) ? es.drag_wall : es.hovered_wall;
                        if (target >= 0) {
                            unsigned int mods = ((XKeyPressedEvent*)key_ev)->state;
                            int shift_down = (mods & ShiftMask) != 0;
                            Wall* w = &level->walls[target];

                            // Toggle culling with 'c'
                            if (keysym == XK_c || keysym == XK_C) w->flip_culling ^= 1;
        
                            // Height adjust: PgUp/PgDn or +/- keys
                            float h_step = 4.0f;
                            if (!shift_down) {
                                if (keysym == XK_plus || 
                                    keysym == XK_KP_Add || 
                                {
                                    w->height = fminf(w->height + h_step, 10000.0f);
                                }
                                if (keysym == XK_minus || 
                                    keysym == XK_KP_Subtract || 
                                {
                                    w->height = fmaxf(w->height - h_step, 0.1f);
                                }
                            }
                            // Shift-modified +/-: move wall vertically (Y)
                            // Detect Shift from the key event state; f
                            // or other paths, consider tracking modifiers explicitly 
                            if (shift_down) {
                                if (keysym == XK_plus || 
                                    keysym == XK_KP_Add) 
                                {
                                    w->pos.y = fminf(w->pos.y - h_step, 10000.0f);
                                }
                                if (keysym == XK_minus || 
                                    keysym == XK_KP_Subtract) 
                                {
                                    w->pos.y = fmaxf(w->pos.y + h_step, -10000.0f);
                                }
                            }
                        }
                    }
                } break;

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
                        if (keysym == XK_Up) keys.up = 0;
                        if (keysym == XK_Down) keys.down = 0;
                        if (keysym == XK_Left) keys.left = 0;
                        if (keysym == XK_Right) keys.right = 0;
                    }
                } break;

                case ConfigureNotify:
                {
                    XConfigureEvent *cfg_ev = (XConfigureEvent *)&ev;
                    win_w = cfg_ev->width;
                    win_h = cfg_ev->height;

                    XDestroyImage(img);
                    free(buf.depth_buffer);

                    buf.w = win_w;
                    buf.h = win_h;
                    buf.pitch = buf.w * bytes_per_px;
                    buf.size = buf.pitch * buf.h;
                    buf.mem = (uint8_t *)malloc(buf.size);
                    buf.depth_buffer = (float *)malloc(sizeof(float) * buf.w * buf.h);

                    img = XCreateImage(
                        disp,
                        vis_info.visual,
                        vis_info.depth,
                        ZPixmap,
                        0,
                        (char *)buf.mem,
                        win_w, win_h,
                        bpp,
                        0);
                } break;

                case MotionNotify:
                {
                    mouse_x = ((XMotionEvent*)&ev)->x;
                    mouse_y = ((XMotionEvent*)&ev)->y;
                    if (editor && es.mode == EMode_DragWall && es.drag_wall >= 0)
                    {
                        static int last_x=-1,last_y=-1;
                        if (!dragging_active)
                        { 
                            last_x = mouse_x; 
                            last_y = mouse_y; 
                            dragging_active=1; 
                        }
                        else 
                        {
                            float wx0,wz0,wxn,wzn;
                            screen_to_map(&es, last_x,last_y,&wx0,&wz0);
                            screen_to_map(&es, mouse_x,mouse_y,&wxn,&wzn);
                            float dx=wxn-wx0, dz=wzn-wz0;
                            level->walls[es.drag_wall].pos.x += dx;
                            level->walls[es.drag_wall].pos.z += dz;
                            last_x = mouse_x; last_y = mouse_y;
                        }
                    }
                    if (editor && es.mode == EMode_DragEndpoint && es.drag_wall >= 0)
                    {
                        float x0,z0,x1,z1; wall_endpoints(&level->walls[es.drag_wall],&x0,&z0,&x1,&z1);
                        float wx,wz; screen_to_map(&es, mouse_x, mouse_y, &wx, &wz);
                        if (es.drag_endpoint==0)
                        { 
                            x0=wx; 
                            z0=wz; 
                        } 
                        else 
                        { 
                            x1=wx; 
                            z1=wz; 
                        }
                        endpoints_to_wall(
                                x0,z0,x1,z1, 
                                level->walls[es.drag_wall].height, 
                                level->walls[es.drag_wall].color, 
                               &level->walls[es.drag_wall]);
                    }
                } break;

                case ButtonPress:
                {
                    XButtonEvent* be = (XButtonEvent*)&ev;
                    mouse_x = be->x; mouse_y = be->y;
                    if (!editor) break;
                    if (be->button == Button1)
                    {
                        if (es.mode == EMode_NewWall)
                        {
                            float wx,wz; screen_to_map(&es, mouse_x,mouse_y,&wx,&wz);
                            if (!es.started)
                            { 
                                es.start_x=wx; 
                                es.start_z=wz; 
                                es.started=1; 
                            }
                            else 
                            {
                                Wall nw; endpoints_to_wall(es.start_x,es.start_z, wx,wz, 40.0f, 0xFFCC4A4A, &nw);
                                level_add_wall(level, nw);
                                es.started=0; es.mode=EMode_Default;
                            }
                        } else 
                        {
                            // pick wall or endpoint
                            int target = es.hovered_wall;
                            if (target>=0){
                                float x0,z0,x1,z1; wall_endpoints(&level->walls[target],&x0,&z0,&x1,&z1);
                                float radius = 9.0f;
                                if (pick_endpoint_px(&es, mouse_x,mouse_y, x0,z0, radius))
                                { 
                                    es.mode=EMode_DragEndpoint; 
                                    es.drag_wall=target; 
                                    es.drag_endpoint=0; 
                                }
                                else if (pick_endpoint_px(&es, mouse_x,mouse_y, x1,z1, radius))
                                { 
                                    es.mode=EMode_DragEndpoint; 
                                    es.drag_wall=target; 
                                    es.drag_endpoint=1; 
                                }
                                else 
                                { 
                                    es.mode=EMode_DragWall; 
                                    es.drag_wall=target; 
                                    es.drag_endpoint=-1; 
                                    dragging_active=0; 
                                }
                            }
                        }
                    }
                } break;

                case ButtonRelease:
                {
                    XButtonEvent* be = (XButtonEvent*)&ev;
                    mouse_x = be->x; mouse_y = be->y;
                    if (!editor) break;
                    if (be->button == Button1)
                    {
                        if (es.mode==EMode_DragWall || es.mode==EMode_DragEndpoint)
                        {
                            es.mode = EMode_Default; 
                            es.drag_wall=-1; 
                            es.drag_endpoint=-1; 
                            dragging_active=0;
                        }
                    }
                } break;
            } // switch
        } // while pending

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
        if ( editor) do_editor(&buf, level, cam, &es, mouse_x, mouse_y);

        XPutImage(disp, win, ctx, img, 0, 0, 0, 0, win_w, win_h);
    } // while(is_open)

    return 0;
}
