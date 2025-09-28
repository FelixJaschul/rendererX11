#ifndef UTIL_H
#define UTIL_H

#include "game.h"

static inline Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static inline float vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec3 vec3_scale(Vec3 v, float s)
{
    return (Vec3){ v.x * s, v.y * s, v.z * s };
}

static inline Vec3 vec3_neg(Vec3 v)
{
    return (Vec3){ -v.x, -v.y, -v.z };
}

static inline float clamp(
        float min, 
        float val, 
        float max)
{
    if (val < min) val = min;
    else if (val > max) val = max;
    return val;
}

static inline float vec3_distance(Vec3 a, Vec3 b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
}

static inline float lerp(
        float a, 
        float b, 
        float t)
{
    return a + (b - a) * t;
}

static inline uint32_t fog_color(
        uint32_t color, 
        uint32_t fog_color, 
        float distance, 
        float fog_near, 
        float fog_far)
{
    float fog_amount = (distance - fog_near) / (fog_far - fog_near);
    fog_amount = clamp(0.0f, fog_amount, 1.0f);

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    uint8_t fog_r = (fog_color >> 16) & 0xFF;
    uint8_t fog_g = (fog_color >> 8) & 0xFF;
    uint8_t fog_b = fog_color & 0xFF;

    uint8_t final_r = (uint8_t)lerp(r, fog_r, fog_amount);
    uint8_t final_g = (uint8_t)lerp(g, fog_g, fog_amount);
    uint8_t final_b = (uint8_t)lerp(b, fog_b, fog_amount);

    return (final_r << 16) | (final_g << 8) | final_b;
}

static inline int is_back_facing(
        Vec3 tri[3], 
        Camera cam)
{
    Vec3 e1 = vec3_sub(tri[1], tri[0]);
    Vec3 e2 = vec3_sub(tri[2], tri[0]);
    Vec3 n  = vec3_cross(e1, e2);
    Vec3 cam_pos = { cam.pos_x, cam.pos_y, cam.pos_z };
    return vec3_dot(n, vec3_sub(cam_pos, tri[0])) <= 0.0f;
}

static inline Vec3 scale_vertex2(Vec3 v, float scale) 
{
    return (Vec3){v.x * scale, v.y * scale, v.z * scale};
}

static inline Vec3 scale_vertex3(Vec3 v, Vec3 scale) 
{
    return (Vec3){v.x * scale.x, v.y * scale.y, v.z * scale.z};
}

static inline Vec3 rotate_vertex3(Vec3 v, float angle) {
    float s = sinf(angle);
    float c = cosf(angle);
    return (Vec3){
        v.x * c - v.z * s,
        v.y,
        v.x * s + v.z * c
    };
}

static inline float distance_from_camera(Vec3 point, Camera cam)
{
    return sqrtf(
        (point.x - cam.pos_x) * (point.x - cam.pos_x) +
        (point.y - cam.pos_y) * (point.y - cam.pos_y) +
        (point.z - cam.pos_z) * (point.z - cam.pos_z)
    );
}

Vec3 (*order[MAX_TRIANGLES]);
int curr_o_n = 0;

static inline Vec3 project(
        Vec3 point, 
        Camera cam, 
        int screen_w, 
        int screen_h) 
{
    float fov = 60.0f * M_PI / 180.0f;
    float focal_length = screen_h / (2.0f * tanf(fov / 2.0f));

    Vec3 relative = {
        point.x - cam.pos_x,
        point.y - cam.pos_y,
        point.z - cam.pos_z
    };

    // Use regular cos/sin for now (can be optimized later)
    float cos_x = cosf(cam.angle_x);
    float sin_x = sinf(cam.angle_x);
    float cos_y = cosf(cam.angle_y);
    float sin_y = sinf(cam.angle_y);

    float temp_x = relative.x * cos_x - relative.z * sin_x;
    float temp_z = relative.x * sin_x + relative.z * cos_x;
    relative.x = temp_x;
    relative.z = temp_z;

    float temp_y = relative.y * cos_y - relative.z * sin_y;
    temp_z = relative.y * sin_y + relative.z * cos_y;
    relative.y = temp_y;
    relative.z = temp_z;

    Vec3 result;
    if (relative.z < 0.1f) relative.z = 0.0001f; 

    float inv_z = 1.0f / relative.z;
    result.x = (relative.x * focal_length * inv_z) + screen_w * 0.5f;
    result.y = (relative.y * focal_length * inv_z) + screen_h * 0.5f;
    result.z = relative.z;

    return result;
}

static inline uint32_t apply_fog(
        uint32_t original_color, 
        float distance) 
{
    if (distance <= g_fog_start) return original_color;
    if (distance >= g_fog_end) return g_fog_color;
    
    float fog_factor = (distance - g_fog_start) / (g_fog_end - g_fog_start);
    fog_factor = fmaxf(0.0f, fminf(1.0f, fog_factor));  
    
    // Use bit operations for faster color blending
    uint32_t r1 = (original_color >> 16) & 0xFF;
    uint32_t g1 = (original_color >> 8) & 0xFF;
    uint32_t b1 = original_color & 0xFF;
    uint32_t r2 = (g_fog_color >> 16) & 0xFF;
    uint32_t g2 = (g_fog_color >> 8) & 0xFF;
    uint32_t b2 = g_fog_color & 0xFF;
    
    uint32_t factor_int = (uint32_t)(fog_factor * 256);
    uint32_t inv_factor = 256 - factor_int;
    
    return (0xFF << 24) |
           (((r1 * inv_factor + r2 * factor_int) >> 8) << 16) |
           (((g1 * inv_factor + g2 * factor_int) >> 8) << 8) |
           ((b1 * inv_factor + b2 * factor_int) >> 8);
}

static inline void place_text(
        Olivec_Canvas oc, 
        char text[32]) 
{
    olivec_text(oc, text, 10, 10, olivec_default_font, 2, 0xFFFFFFFF);
}

static inline void place_triangle3c(
        buffer *buf, 
        Olivec_Canvas oc, 
        Vec3 tri[3],
        uint32_t c0, 
        uint32_t c1, 
        uint32_t c2, 
        Camera cam) 
{
    if (curr_o_n == MAX_TRIANGLES) return;
    order[curr_o_n] = tri;

    if (is_back_facing(tri, cam)) return;

    // Early distance culling
    float avg_distance = (
        distance_from_camera(tri[0], cam) +
        distance_from_camera(tri[1], cam) +
        distance_from_camera(tri[2], cam)
    ) / 3.0f;
    
    if (avg_distance > g_fog_end) return; // Skip triangles beyond fog

    Vec3 screen[3];
    int visible = 0;
    for (int i = 0; i < 3; i++) {
        screen[i] = project(tri[i], cam, buf->w, buf->h);
        // Check if at least one vertex is in screen bounds
        if (screen[i].x >= -100 && screen[i].x <= buf->w + 100 &&
            screen[i].y >= -100 && screen[i].y <= buf->h + 100 &&
            screen[i].z > 0.1f) {
            visible = 1;
        }
    }
    
    if (!visible) return;
        
    olivec_triangle3c(oc,
            (int)screen[0].x, (int)screen[0].y,
            (int)screen[1].x, (int)screen[1].y,
            (int)screen[2].x, (int)screen[2].y,
            apply_fog(c0, avg_distance), 
            apply_fog(c1, avg_distance), 
            apply_fog(c2, avg_distance));
}

static inline void place_triangle1c(
        buffer *buf, 
        Olivec_Canvas oc, 
        Vec3 tri[3], 
        uint32_t color, 
        Camera cam) 
{
    place_triangle3c(buf, oc, tri, color, color, color, cam);    
}

static inline void place_rect(
        buffer *buf,
        Olivec_Canvas oc,
        Vec3 v0,
        Vec3 v1,
        Vec3 v2,
        Vec3 v3,
        uint32_t c,
        Camera cam)
{
    // Quick distance check using rectangle center
    Vec3 center = {
        (v0.x + v1.x + v2.x + v3.x) * 0.25f,
        (v0.y + v1.y + v2.y + v3.y) * 0.25f,
        (v0.z + v1.z + v2.z + v3.z) * 0.25f
    };

    float dist_sq = (center.x - cam.pos_x) * (center.x - cam.pos_x) +
                    (center.y - cam.pos_y) * (center.y - cam.pos_y) +
                    (center.z - cam.pos_z) * (center.z - cam.pos_z);

    if (dist_sq > g_fog_end * g_fog_end) return;

    // Split rectangle into two triangles
    Vec3 tri1[3] = {v0, v1, v2};
    Vec3 tri2[3] = {v0, v2, v3};

    place_triangle1c(buf, oc, tri1, c, cam);
    place_triangle1c(buf, oc, tri2, c, cam);
}

static inline void create_background(
        Olivec_Canvas oc,
        uint32_t c)
{
     olivec_fill(oc, c); 
}

static inline void create_floor(
        buffer *buf,
        Olivec_Canvas oc,
        int tile_count_x,
        int tile_count_z,
        float tile_size,
        float floor_y,
        Camera cam)
{ 
    Vec3 cam_pos = {cam.pos_x, cam.pos_y, cam.pos_z};

    for (int tz = 0; tz < tile_count_z; tz++) 
    {
        for (int tx = -tile_count_x/2; tx < tile_count_x/2; tx++) 
        {
            float x0 = tx * tile_size;
            float z0 = tz * tile_size + 4.0f - 500.0f;
            float y  = floor_y;

            Vec3 v0 = { x0,         y, z0          };
            Vec3 v1 = { x0 + tile_size, y, z0          };
            Vec3 v2 = { x0 + tile_size, y, z0 + tile_size };
            Vec3 v3 = { x0,         y, z0 + tile_size };

            uint32_t color = ((tx + tz) & 1) ? 0xff000000 : 0xFFaaddff;

            place_rect(buf, oc, v0, v1, v2, v3, color, cam);
        }
    }
}

static inline void update_camera(
        Camera* cam, 
        KeyState* keys, 
        float dt) 
{
    float move_speed = 200.0f;
    float rotate_speed = 2.0f;

    if (keys->left) cam->angle_x -= rotate_speed * dt;
    if (keys->right) cam->angle_x += rotate_speed * dt;
    if (keys->up) cam->angle_y -= rotate_speed * dt;
    if (keys->down) cam->angle_y += rotate_speed * dt;

    cam->angle_y = clamp(-M_PI/2 + 0.1f, cam->angle_y, M_PI/2 - 0.1f);

    float cos_x = cosf(cam->angle_x);
    float sin_x = sinf(cam->angle_x);

    Vec3 forward = {sin_x, 0, cos_x};
    Vec3 right = {cos_x, 0, -sin_x};

    float move_delta = move_speed * dt;

    if (keys->w) 
    {
        cam->pos_x += forward.x * move_delta;
        cam->pos_z += forward.z * move_delta;
    }
    if (keys->s) 
    {
        cam->pos_x -= forward.x * move_delta;
        cam->pos_z -= forward.z * move_delta;
    }
    if (keys->a) 
    {
        cam->pos_x -= right.x * move_delta;
        cam->pos_z -= right.z * move_delta;
    }
    if (keys->d) 
    {
        cam->pos_x += right.x * move_delta;
        cam->pos_z += right.z * move_delta;
    }

    if (keys->space) cam->pos_y -= move_delta;
    if (keys->shift) cam->pos_y += move_delta;
}

#endif // UTIL_H
