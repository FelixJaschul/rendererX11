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

#define g_fog_start 20000.0f
#define g_fog_end 1000.0f
#define g_fog_color 0xFFffaaee

typedef struct
{
    float x, y;
} Vec2;

typedef struct
{
    float x, y, z;
} Vec3;

typedef struct
{
    uint8_t *mem;
    float *depth_buffer;
    uint32_t w;
    uint32_t h;
    uint64_t size;
    uint32_t pitch;
} buffer;

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
} Camera;

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

#define NANO() \
    ({ struct timespec ts; \
       clock_gettime(CLOCK_MONOTONIC, &ts); \
       (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec; })

#endif // GAME_H
