#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"

static inline Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    Vec3 result = { dx, dy, dz };
    return result;
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b)
{
    float dx = a.x + b.x;
    float dy = a.y + b.y;
    float dz = a.z + b.z;
    Vec3 result = { dx, dy, dz };
    return result;
}

static inline Vec3 vec3_scale(Vec3 v, float s)
{
    float sx = v.x * s;
    float sy = v.y * s;
    float sz = v.z * s;
    Vec3 result = { sx, sy, sz };
    return result;
}

static inline Vec3 vec3_neg(Vec3 v)
{
    float nx = -v.x;
    float ny = -v.y;
    float nz = -v.z;
    Vec3 result = { nx, ny, nz };
    return result;
}

static inline float vec3_dot(Vec3 a, Vec3 b)
{
    float dx = a.x * b.x;
    float dy = a.y * b.y;
    float dz = a.z * b.z;
    float result = dx + dy + dz;
    return result;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    float cx = a.y * b.z - a.z * b.y;
    float cy = a.z * b.x - a.x * b.z;
    float cz = a.x * b.y - a.y * b.x;
    Vec3 result = { cx, cy, cz };
    return result;
}

static inline float clamp(float min, float val, float max)
{
    float result = val;
    if (val < min) result = min;
    else if (val > max) result = max;
    return result;
}

static inline Vec3 vec3_normalize(Vec3 v)
{
    float len_sq = v.x*v.x + v.y*v.y + v.z*v.z;
    float len = sqrtf(len_sq);
    if (len < 0.0001f) return (Vec3){0,0,0};
    float nx = v.x / len;
    float ny = v.y / len;
    float nz = v.z / len;
    Vec3 result = { nx, ny, nz };
    return result;
}

static inline Vec3 calculate_triangle_normal(Vec3 tri[3])
{
    Vec3 e1 = vec3_sub(tri[1], tri[0]);
    Vec3 e2 = vec3_sub(tri[2], tri[0]);
    Vec3 n = vec3_cross(e1, e2);
    Vec3 normal = vec3_normalize(n);
    return normal;
}

static inline float vec3_distance(Vec3 a, Vec3 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    float dist_sq = dx*dx + dy*dy + dz*dz;
    float dist = sqrtf(dist_sq);
    return dist;
}

static inline float lerp(float a, float b, float t)
{
    float diff = b - a;
    float result = a + diff * t;
    return result;
}

static inline void put_pixel_depth(buffer *buf, int x, int y, float z, uint32_t c)
{
    if (x < 0 || y < 0 || x >= (int)buf->w || y >= (int)buf->h) return;
    int idx = y * buf->w + x;
    if (z < buf->depth_buffer[idx])
    {
        buf->depth_buffer[idx] = z;
        ((uint32_t*)buf->mem)[idx] = c;
    }
}

static inline uint32_t fog_color(uint32_t color, uint32_t fog_color, float distance, float fog_near, float fog_far)
{
    float fog_amount = (distance - fog_near) / (fog_far - fog_near);
    fog_amount = clamp(0.0f, fog_amount, 1.0f);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t fr = (fog_color >> 16) & 0xFF;
    uint8_t fg = (fog_color >> 8) & 0xFF;
    uint8_t fb = fog_color & 0xFF;
    uint8_t final_r = (uint8_t)lerp(r, fr, fog_amount);
    uint8_t final_g = (uint8_t)lerp(g, fg, fog_amount);
    uint8_t final_b = (uint8_t)lerp(b, fb, fog_amount);
    uint32_t result = (final_r << 16) | (final_g << 8) | final_b;
    return result;
}

static inline Vec3 light_space(Vec3 world, Light light)
{
    Vec3 inv_dir = vec3_neg(light.direction);
    Vec3 dir = vec3_normalize(inv_dir);

    Vec3 up = {0,1,0};
    Vec3 right = vec3_cross(up, dir);
    right = vec3_normalize(right);
    Vec3 new_up = vec3_cross(dir, right);

    float lx = vec3_dot(world, right);
    float ly = vec3_dot(world, new_up);
    float lz = vec3_dot(world, dir);
    Vec3 result = { lx, ly, lz };
    return result;
}

static inline int is_back_facing(Vec3 tri[3], Camera cam)
{
    Vec3 e1 = vec3_sub(tri[1], tri[0]);
    Vec3 e2 = vec3_sub(tri[2], tri[0]);
    Vec3 n = vec3_cross(e1, e2);

    Vec3 cam_pos = { cam.pos_x, cam.pos_y, cam.pos_z };
    Vec3 dir = vec3_sub(cam_pos, tri[0]);
    float d = vec3_dot(n, dir);
    return d <= 0.0f;
}

static inline Vec3 scale_vertex2(Vec3 v, float scale)
{
    float sx = v.x * scale;
    float sy = v.y * scale;
    float sz = v.z * scale;
    Vec3 result = { sx, sy, sz };
    return result;
}

static inline Vec3 scale_vertex3(Vec3 v, Vec3 scale)
{
    float sx = v.x * scale.x;
    float sy = v.y * scale.y;
    float sz = v.z * scale.z;
    Vec3 result = { sx, sy, sz };
    return result;
}

static inline Vec3 rotate_vertex3(Vec3 v, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);
    float rx = v.x * c - v.z * s;
    float ry = v.y;
    float rz = v.x * s + v.z * c;
    Vec3 result = { rx, ry, rz };
    return result;
}

static inline float distance_from_camera(Vec3 point, Camera cam)
{
    float dx = point.x - cam.pos_x;
    float dy = point.y - cam.pos_y;
    float dz = point.z - cam.pos_z;
    float dist_sq = dx*dx + dy*dy + dz*dz;
    float dist = sqrtf(dist_sq);
    return dist;
}

static int cmp_depth(const void *a, const void *b)
{
    const RenderTri *ra = a;
    const RenderTri *rb = b;
    return (ra->depth < rb->depth) ? 1 : (ra->depth > rb->depth) ? -1 : 0;
}

static inline Vec3 project(Vec3 point, Camera cam, int screen_w, int screen_h)
{
    float fov = 60.0f * M_PI / 180.0f;
    float focal_length = screen_h / (2.0f * tanf(fov / 2.0f));

    Vec3 rel = { point.x - cam.pos_x, point.y - cam.pos_y, point.z - cam.pos_z };

    float cos_x = cosf(cam.angle_x);
    float sin_x = sinf(cam.angle_x);
    float cos_y = cosf(cam.angle_y);
    float sin_y = sinf(cam.angle_y);

    float tmp_x = rel.x * cos_x - rel.z * sin_x;
    float tmp_z = rel.x * sin_x + rel.z * cos_x;
    rel.x = tmp_x;
    rel.z = tmp_z;

    float tmp_y = rel.y * cos_y - rel.z * sin_y;
    tmp_z = rel.y * sin_y + rel.z * cos_y;
    rel.y = tmp_y;
    rel.z = tmp_z;

    if (rel.z < Z_NEAR) rel.z = Z_NEAR;

    float inv_z = 1.0f / rel.z;
    float sx = (rel.x * focal_length * inv_z) + screen_w * 0.5f;
    float sy = (rel.y * focal_length * inv_z) + screen_h * 0.5f;
    float sz = (rel.z - Z_NEAR) / (Z_FAR - Z_NEAR);

    Vec3 result = { sx, sy, sz };
    return result;
}

static inline uint32_t apply_fog(uint32_t original_color, float distance)
{
    if (distance <= g_fog_start) return original_color;
#ifdef g_fog_active
    if (distance >= g_fog_end) return g_fog_color;

    float fog_factor = (distance - g_fog_start) / (g_fog_end - g_fog_start);
    fog_factor = fmaxf(0.0f, fminf(1.0f, fog_factor));

    uint32_t r1 = (original_color >> 16) & 0xFF;
    uint32_t g1 = (original_color >> 8) & 0xFF;
    uint32_t b1 = original_color & 0xFF;

    uint32_t r2 = (g_fog_color >> 16) & 0xFF;
    uint32_t g2 = (g_fog_color >> 8) & 0xFF;
    uint32_t b2 = g_fog_color & 0xFF;

    uint32_t factor_int = (uint32_t)(fog_factor * 256);
    uint32_t inv_factor = 256 - factor_int;

    uint32_t nr = ((r1 * inv_factor + r2 * factor_int) >> 8);
    uint32_t ng = ((g1 * inv_factor + g2 * factor_int) >> 8);
    uint32_t nb = ((b1 * inv_factor + b2 * factor_int) >> 8);

    uint32_t result = (0xFF << 24) | (nr << 16) | (ng << 8) | nb;
    return result;
#endif
    return original_color;
}

static inline uint32_t apply_lighting(uint32_t base_color, Vec3 surface_normal, Vec3 surface_position, Light light)
{
#ifdef g_light_active
    Vec3 light_dir;
    if (light.is_directional)
    {
        light_dir = vec3_neg(light.direction);
    }
    else
    {
        light_dir = vec3_sub(light.position, surface_position);
    }
    light_dir = vec3_normalize(light_dir);

    float diffuse = vec3_dot(surface_normal, light_dir);
    diffuse = fmaxf(0.0f, diffuse);

    float ambient = 0.2f;
    float intensity = ambient + diffuse * light.intensity * (1.0f - ambient);

    uint8_t r = ((base_color >> 16) & 0xFF);
    uint8_t g = ((base_color >> 8) & 0xFF);
    uint8_t b = (base_color & 0xFF);

    r = (uint8_t)(r * intensity);
    g = (uint8_t)(g * intensity);
    b = (uint8_t)(b * intensity);

    uint8_t lr = (light.color >> 16) & 0xFF;
    uint8_t lg = (light.color >> 8) & 0xFF;
    uint8_t lb = light.color & 0xFF;

    r = (r * lr) >> 8;
    g = (g * lg) >> 8;
    b = (b * lb) >> 8;

    uint32_t result = 0xFF000000 | (r << 16) | (g << 8) | b;
    return result;
#endif
    return base_color;
}

static inline void place_text(Olivec_Canvas oc, char text[2048])
{
    olivec_text(oc, text, 10, 10, olivec_default_font, 2, 0xFFFFFFFF);
}

static inline void place_triangle( buffer *buf, Olivec_Canvas oc, Vec3 tri[3], uint32_t c, Light light, Camera cam)
{
    if (is_back_facing(tri, cam)) return;
    Vec3 normal = calculate_triangle_normal(tri);
    Vec3 center = {
        (tri[0].x + tri[1].x + tri[2].x) / 3.0f,
        (tri[0].y + tri[1].y + tri[2].y) / 3.0f,
        (tri[0].z + tri[1].z + tri[2].z) / 3.0f
    };

    uint32_t lit_color = apply_lighting(c, normal, center, light);

    Vec3 screen[3];
    float cam_z[3];
    for (int i = 0; i < 3; i++)
    {
        screen[i] = project(tri[i], cam, buf->w, buf->h);
        cam_z[i] = tri[i].z - cam.pos_z;
    }

    int minX = fmaxf(0, floorf(fminf(screen[0].x, fminf(screen[1].x, screen[2].x))));
    int minY = fmaxf(0, floorf(fminf(screen[0].y, fminf(screen[1].y, screen[2].y))));
    int maxX = fminf(buf->w - 1, ceilf(fmaxf(screen[0].x, fmaxf(screen[1].x, screen[2].x))));
    int maxY = fminf(buf->h - 1, ceilf(fmaxf(screen[0].y, fmaxf(screen[1].y, screen[2].y))));

    float denom = ((screen[1].y - screen[2].y)*(screen[0].x - screen[2].x) +
                   (screen[2].x - screen[1].x)*(screen[0].y - screen[2].y));
    if (fabsf(denom) < 1e-6f) return;

    for (int y = minY; y <= maxY; y++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            float alpha = ((screen[1].y - screen[2].y)*(x - screen[2].x) +
                           (screen[2].x - screen[1].x)*(y - screen[2].y)) / denom;
            float beta = ((screen[2].y - screen[0].y)*(x - screen[2].x) +
                          (screen[0].x - screen[2].x)*(y - screen[2].y)) / denom;
            float gamma = 1.0f - alpha - beta;

            if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
                float z =
                    alpha * screen[0].z + beta * screen[1].z + gamma * screen[2].z;

                float dist = distance_from_camera(center, cam);
                uint32_t fog_color_val = apply_fog(lit_color, dist);

                put_pixel_depth(buf, x, y, z, fog_color_val);
            }
        }
    }
}

static inline void place_rect_help( buffer *buf, Olivec_Canvas oc, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3, uint32_t c, Light light, Camera cam)
{
    Vec3 center = {
        (v0.x + v1.x + v2.x + v3.x) * 0.25f,
        (v0.y + v1.y + v2.y + v3.y) * 0.25f,
        (v0.z + v1.z + v2.z + v3.z) * 0.25f
    };
    float dist_sq = (center.x - cam.pos_x) * (center.x - cam.pos_x) +
                    (center.y - cam.pos_y) * (center.y - cam.pos_y) +
                    (center.z - cam.pos_z) * (center.z - cam.pos_z);
    if (dist_sq > g_fog_end * g_fog_end) return;

    Vec3 tri1[3] = {v0, v1, v2};
    Vec3 tri2[3] = {v0, v2, v3};
    place_triangle(buf, oc, tri1, c, light, cam);
    place_triangle(buf, oc, tri2, c, light, cam);
}

static inline void place_rect( buffer *buf, Olivec_Canvas oc, Vec3 pos, float size1, float size2, float angle, uint32_t c, RectType type, bool cull_other_side, Light light, Camera cam)
{
    Vec3 v0, v1, v2, v3;
    switch (type)
    {
        case FLOOR:
            v0 = (Vec3){0, 0, 0};
            v1 = (Vec3){size1, 0, 0};
            v2 = (Vec3){size1, 0, size2};
            v3 = (Vec3){0, 0, size2};
            break;
        case WALL_X:
            v0 = (Vec3){0, 0, 0};
            v1 = (Vec3){0, 0, size1};
            v2 = (Vec3){0, size2, size1};
            v3 = (Vec3){0, size2, 0};
            break;
        case WALL_Z:
            v0 = (Vec3){0, 0, 0};
            v1 = (Vec3){size1, 0, 0};
            v2 = (Vec3){size1, size2, 0};
            v3 = (Vec3){0, size2, 0};
            break;
    }

    Vec3 verts[4] = {v0, v1, v2, v3};

    for (int i = 0; i < 4; i++)
    {
        float x = verts[i].x;
        float z = verts[i].z;
        verts[i].x = x * cosf(angle) - z * sinf(angle);
        verts[i].z = x * sinf(angle) + z * cosf(angle);

        verts[i].y += pos.y;
        verts[i].x += pos.x;
        verts[i].z += pos.z;
    }

    if (cull_other_side)
    {
        Vec3 tmp = verts[1];
        verts[1] = verts[3];
        verts[3] = tmp;
    }

    place_rect_help(buf, oc, verts[0], verts[1], verts[2], verts[3], c, light, cam);
}

static inline void create_background(Olivec_Canvas oc, uint32_t c)
{
    olivec_fill(oc, c);
}

static inline void create_floor(buffer *buf,Olivec_Canvas oc,int tile_count_x, int tile_count_z, float tile_size, float floor_y, uint32_t c1, uint32_t c2, Light light, Camera cam)
{
    for (int tz = 0; tz < tile_count_z; tz++)
    {
        for (int tx = -tile_count_x / 2; tx < tile_count_x / 2; tx++)
        {
            Vec3 pos = {
                tx * tile_size - 1000, // -1000 so the we dont just see one end of the tiles in the middle of the map
                // its a made up number lol
                floor_y,
                tz * tile_size + 4.0f - 500.0f - 1000
            };
            uint32_t color = ((tx + tz) & 1) ? c1 : c2;
            place_rect(
                buf,
                oc,
                pos,
                tile_size,
                tile_size,
                0.0f,
                color,
                FLOOR,
                false,
                light,
                cam
            );
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

    if (keys->left)  cam->angle_x -= rotate_speed * dt;
    if (keys->right) cam->angle_x += rotate_speed * dt;
    if (keys->up)    cam->angle_y -= rotate_speed * dt;
    if (keys->down)  cam->angle_y += rotate_speed * dt;

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

#endif
