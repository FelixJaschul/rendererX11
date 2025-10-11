#ifndef EDITOR_H
#define EDITOR_H

#include "util.h"
#include "game.h"


// Float PI to avoid double->float warnings with fabsf
#ifndef PI_F
#define PI_F 3.14159265358979323846f
#endif

typedef enum { EMode_Default, EMode_NewWall, EMode_DragWall, EMode_DragEndpoint } EMode;

typedef struct {
    float scale;      // pixels per world unit
    float pan_x;      // world-space x shown at screen 0
    float pan_z;      // world-space z shown at screen 0
    int hovered_wall; // -1 if none
    int drag_wall;    // index being dragged
    int drag_endpoint;// -1 none, 0 or 1 endpoint
    float start_x, start_z; // NewWall first click
    int started;      // NewWall started
    int snap_active;
    float snap_size;
    int viewport_focused;
    EMode mode;
} EditorState;

typedef struct {
    int x, y, w, h;  // viewport boundaries
} Viewport;

static inline float snapf(float v, float step)
{
    return (step > 1e-6f) ? roundf(v / step) * step : v;
}

static inline void wall_endpoints(const Wall* w, float* x0,float* z0,float* x1,float* z1)
{
    float a=w->angle;
    float dx=(w->type==WALL_Z)?w->width:0.0f;
    float dz=(w->type==WALL_X)?w->width:0.0f;
    float rx = dx*cosf(a) - dz*sinf(a);
    float rz = dx*sinf(a) + dz*cosf(a);
    *x0 = w->pos.x; *z0 = w->pos.z; *x1 = w->pos.x + rx; *z1 = w->pos.z + rz;
}

static inline void endpoints_to_wall(float x0,float z0,float x1,float z1, float default_h, uint32_t col, Wall* out)
{
    float dx = x1-x0, dz = z1-z0;
    float len = sqrtf(dx*dx+dz*dz);
    if (len < 1e-4f) len = 1e-4f;
    float ang = atan2f(dz, dx) - (PI_F * 0.5f);
    if (ang > PI_F)  ang -= 2.0f * PI_F;
    if (ang < -PI_F) ang += 2.0f * PI_F;
    out->pos = (Vec3){ x0, 0.0f, z0 };
    out->width = len;
    out->height = default_h;
    out->angle = ang;
    // classify predominantly X vs Z alignment for quick type
    float aabs = fabsf(ang);
    out->type = (fabsf(aabs - 0.0f) < 0.001f || fabsf(aabs - PI_F) < 0.001f) ? WALL_Z : WALL_X;
    out->color = col;
    out->flip_culling = 0;
}

static inline void editor_fit_level(EditorState* es, const buffer* buf, const Level* level)
{
    float minx=1e9f,maxx=-1e9f,minz=1e9f,maxz=-1e9f; int any=0;
    for (int i=0;i<level->wall_count;i++)
    {
        const Wall* w=&level->walls[i];
        if (w->type==FLOOR) continue;
        float x0,z0,x1,z1; wall_endpoints(w,&x0,&z0,&x1,&z1);
        minx=fminf(minx,fminf(x0,x1)); maxx=fmaxf(maxx,fmaxf(x0,x1));
        minz=fminf(minz,fminf(z0,z1)); maxz=fmaxf(maxz,fmaxf(z0,z1)); any=1;
    }
    if (!any){ es->scale=40.0f; es->pan_x=-buf->w*0.5f/es->scale; es->pan_z=-buf->h*0.5f/es->scale; return; }
    const float margin_px=40.0f;
    float sx=(buf->w-2*margin_px)/fmaxf(1e-3f,(maxx-minx));
    float sy=(buf->h-2*margin_px)/fmaxf(1e-3f,(maxz-minz));
    es->scale=fminf(sx,sy);
    float cx=0.5f*(minx+maxx), cz=0.5f*(minz+maxz);
    float scr_cx=buf->w*0.5f, scr_cz=buf->h*0.5f;
    es->pan_x = cx - scr_cx/es->scale;
    es->pan_z = cz - scr_cz/es->scale;
}

static inline void map_to_screen(const EditorState* es, float wx,float wz, int* sx,int* sy)
{
    *sx = (int)((wx - es->pan_x)*es->scale);
    *sy = (int)((wz - es->pan_z)*es->scale);
}
static inline void screen_to_map(const EditorState* es, int sx,int sy, float* wx,float* wz)
{
    *wx = es->pan_x + (float)sx/es->scale;
    *wz = es->pan_z + (float)sy/es->scale;
}

static inline void map_to_screen_vp(const EditorState* es, const Viewport* vp, float wx, float wz, int* sx, int* sy)
{
    *sx = vp->x + (int)((wx - es->pan_x) * es->scale);
    *sy = vp->y + (int)((wz - es->pan_z) * es->scale);
}
static inline void screen_to_map_vp(const EditorState* es, const Viewport* vp, int sx, int sy, float* wx, float* wz)
{
    *wx = es->pan_x + (float)(sx - vp->x) / es->scale;
    *wz = es->pan_z + (float)(sy - vp->y) / es->scale;
}

// Check if mouse is inside 2D viewport
static inline int mouse_in_viewport(int mouse_x, int mouse_y, const Viewport* vp)
{
    return (mouse_x >= vp->x && mouse_x < vp->x + vp->w &&
            mouse_y >= vp->y && mouse_y < vp->y + vp->h);
}

static inline float seg_dist_px(const EditorState* es, int mx,int my, float x0,float z0,float x1,float z1)
{
    int sx0,sy0,sx1,sy1; map_to_screen(es,x0,z0,&sx0,&sy0); map_to_screen(es,x1,z1,&sx1,&sy1);
    float vx=sx1-sx0, vy=sy1-sy0, wx=mx-sx0, wy=my-sy0;
    float vv = vx*vx+vy*vy; if (vv<1e-6f) vv=1.0f;
    float t=fmaxf(0.0f,fminf(1.0f,(vx*wx+vy*wy)/vv));
    float cx=sx0 + t*vx, cy=sy0 + t*vy;
    float dx=mx-cx, dy=my-cy; return sqrtf(dx*dx+dy*dy);
}

static inline int pick_endpoint_px_vp(const EditorState* es, const Viewport* vp, int mx, int my, float wx, float wz, float radius_px)
{
    int sx, sy;
    map_to_screen_vp(es, vp, wx, wz, &sx, &sy);
    float dx = mx - sx;
    float dy = my - sy;
    return (dx*dx + dy*dy) <= radius_px*radius_px;
}

static inline void get_editor_viewports(int screen_w, int screen_h, Viewport* vp_3d, Viewport* vp_2d, Viewport* vp_info)
{
    // 3D viewport: top-left, takes 50% width, 100% height
    vp_3d->x = 0;
    vp_3d->y = 0;
    vp_3d->w = screen_w / 2;
    vp_3d->h = screen_h;
    
    // 2D map viewport: top-right, takes 50% width, 70% height
    vp_2d->x = screen_w / 2;
    vp_2d->y = 0;
    vp_2d->w = screen_w / 2;
    vp_2d->h = (int)(screen_h * 0.7f);
    
    // Info panel: bottom-right, takes 50% width, 30% height
    vp_info->x = screen_w / 2;
    vp_info->y = (int)(screen_h * 0.7f);
    vp_info->w = screen_w / 2;
    vp_info->h = screen_h - (int)(screen_h * 0.7f);
}

static inline void render_info_panel(Olivec_Canvas oc, Viewport vp, EditorState* es, Level* level)
{
    // Dark background for info panel
    olivec_rect(oc, vp.x, vp.y, vp.w, vp.h, 0xFF1a1a1a);
    
    int text_x = vp.x + 10;
    int text_y = vp.y + 20;
    int line_height = 20;
    char info_text[256];
    
    if (es->hovered_wall >= 0 && es->hovered_wall < level->wall_count) {
        Wall* w = &level->walls[es->hovered_wall];
        
        snprintf(info_text, sizeof(info_text), "hovered wall id: %d", es->hovered_wall);
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFFFFFFFF);
        text_y += line_height;
        
        snprintf(info_text, sizeof(info_text), "pos: (%.1f, %.1f, %.1f)", w->pos.x, w->pos.y, w->pos.z);
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFFFFFFFF);
        text_y += line_height;
        
        snprintf(info_text, sizeof(info_text), "width: %.1f  height: %.1f", w->width, w->height);
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFFFFFFFF);
        text_y += line_height;
        
        snprintf(info_text, sizeof(info_text), "angle: %.2f  type: %s", 
                 w->angle, w->type == WALL_X ? "wallx" : w->type == WALL_Z ? "wallz" : "floor");
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFFFFFFFF);
        text_y += line_height;
        
        snprintf(info_text, sizeof(info_text), "culling: %s", w->flip_culling ? "flipped" : "normal");
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFFEAD14B);
        text_y += line_height;
    }
    
    if (es->drag_wall >= 0 && es->drag_wall < level->wall_count) {
        text_y += 10;
        snprintf(info_text, sizeof(info_text), "selected wall id: %d", es->drag_wall);
        olivec_text(oc, info_text, text_x, text_y, olivec_default_font, 2, 0xFF3B82F6);
    }
    
    // Mode indicator
    text_y = vp.y + vp.h - 30;
    const char* mode_str = "mode: default";
    if (es->mode == EMode_NewWall) mode_str = "mode: New wall";
    else if (es->mode == EMode_DragWall) mode_str = "mode: drag wall";
    else if (es->mode == EMode_DragEndpoint) mode_str = "mode: drag endpoint";
    olivec_text(oc, mode_str, text_x, text_y, olivec_default_font, 2, 0xFF00FF00);
}

#endif // EDITOR_H
