#ifndef GAME_H
#define GAME_H

#include <time.h>
#include <math.h>
#include <stdint.h>

#define PI M_PI
#define TAU (2.0f * M_PI)

#define FPS 60
#define STATUS_ERROR 0
#define DELTA_TIME 1e9/FPS

#define Z_NEAR 0.1f
#define Z_FAR 1000.0f
#define MAX_POLY_VERTS 10
#define MAX_TRIANGLES 1028

#define g_light_active
#define g_fog_active
#define g_fog_start 400.0f
#define g_fog_end 1000.0f
#define g_fog_color 0xFF78de99 // 0xFF87de87 // 0xFF000000 // 0xFFffaaee

typedef enum { FLOOR, WALL_X, WALL_Z } RectType;

typedef struct
{
    float x;
    float y;
} 
Vec2;

typedef struct
{
    float x;
    float y;
    float z;
} 
Vec3;

typedef struct
{
    uint8_t *mem;
    float *depth_buffer;
    float *shadow_depth;
    int shadow_w;
    int shadow_h;
    uint32_t w;
    uint32_t h;
    uint64_t size;
    uint32_t pitch;
} 
buffer;

typedef struct 
{
    Vec3 tri[3];
    uint32_t color;
    float depth;
} 
RenderTri;

typedef struct
{
    float distance;
    float angle_x;
    float angle_y;
    float pos_x;
    float pos_y;
    float pos_z;
    float fov;
    float aspect_ratio;
} 
Camera;

typedef struct
{
    Vec3 pos;
    float width;
    float height;
    float angle;
    RectType type;
    uint32_t color;
    int flip_culling;
}
Wall;

typedef struct
{
    Wall* walls;
    int wall_count;
    int wall_capacity;
}
Level;

typedef struct 
{
    Vec3 verts[4]; 
    uint32_t color;
    float dist;
}
Face;

typedef struct
{
    int w, a, s, d;
    int up, down, left, right;
    int shift, space;
}
KeyState;

typedef struct
{
    Vec3 vertices[MAX_POLY_VERTS];
    int num_vertices;
}
Polygon;

typedef struct
{
    Vec3 normal;
    float distance;
}
Plane;

typedef struct
{
    Vec3 position;
    Vec3 direction;
    uint32_t color;
    float intensity;
    int is_directional;
}
Light;

#define NANO() \
    ({ struct timespec ts; \
       clock_gettime(CLOCK_MONOTONIC, &ts); \
       (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec; })

#endif // GAME_H
