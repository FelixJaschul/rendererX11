#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "util.h"

static inline int triangle_behind_camera(Vec3 tri[3], Camera cam)
{
    for (int i = 0; i < 3; i++)
    {
        Vec3 rel = { tri[i].x - cam.pos_x, tri[i].y - cam.pos_y, tri[i].z - cam.pos_z };
        
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

        // If any vertex is behind or at camera, reject triangle
        if (rel.z <= Z_NEAR * 0.5f) return 1;
    }
    return 0;
}

static inline void place_triangle( buffer *buf, Olivec_Canvas oc, Vec3 tri[3], uint32_t c, Light light, Camera cam)
{
    if (is_back_facing(tri, cam)) return;
    if (triangle_behind_camera(tri, cam)) return;

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

#endif // TRIANGLE_H
