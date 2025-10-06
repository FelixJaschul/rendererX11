#ifndef UTIL_H
#define UTIL_H

#include "game.h"
#include "math.h"

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
    uint8_t r  = (color >> 16) & 0xFF;
    uint8_t g  = (color >> 8) & 0xFF;
    uint8_t b  = color & 0xFF;
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
