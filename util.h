#ifndef UTIL_H
#define UTIL_H

#include <string.h>

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

static inline void put_pixel_depth(
        buffer *buf, 
        int x, int y, 
        float z, 
        uint32_t color)
{
    if (x < 0 || y < 0 || x >= buf->w || y >= buf->h) 
        return;

    int idx = y * buf->w + x;
    if (z < buf->depth_buffer[idx])
    {
        buf->depth_buffer[idx] = z;
        ((uint32_t*)buf->mem)[idx] = color;
    }
}


static inline Vec3 vec3_normalize(Vec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.0001f) return (Vec3){0, 0, 0};
    return (Vec3){v.x / len, v.y / len, v.z / len};
}

static inline Vec3 calculate_triangle_normal(Vec3 tri[3])
{
    Vec3 e1 = vec3_sub(tri[1], tri[0]);
    Vec3 e2 = vec3_sub(tri[2], tri[0]);
    Vec3 normal = vec3_cross(e1, e2);
    return vec3_normalize(normal);
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

static inline Vec3 light_space(
        Vec3 world, 
        Light light)
{
    // directional light: assume orthographic camera along -light.direction
    Vec3 dir = vec3_normalize(vec3_neg(light.direction));

    Vec3 up = {0,1,0};
    Vec3 right = vec3_normalize(vec3_cross(up, dir));
    up = vec3_cross(dir, right);

    // world position into light space coordinates
    return (Vec3){
        vec3_dot(world, right),
        vec3_dot(world, up),
        vec3_dot(world, dir)  // depth along light
    };
}

static inline int is_back_facing( 
        Vec3 tri[3], 
        Camera cam) 
{ 
    Vec3 e1 = vec3_sub(tri[1], tri[0]); 
    Vec3 e2 = vec3_sub(tri[2], tri[0]); 
    Vec3 n = vec3_cross(e1, e2); 
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

static int cmp_depth(const void *a, const void *b)
{
    const RenderTri *ra = a, *rb = b;
    // furthest first
    return (ra->depth < rb->depth) ? 1 :
           (ra->depth > rb->depth) ? -1 : 0;
}

static inline Vec3 project(Vec3 point, Camera cam, int screen_w, int screen_h)
{
    float fov = 60.0f * M_PI / 180.0f;
    float focal_length = screen_h / (2.0f * tanf(fov / 2.0f));

    // Relative position to camera
    Vec3 rel = {
        point.x - cam.pos_x,
        point.y - cam.pos_y,
        point.z - cam.pos_z
    };

    // Rotate around Y (angle_x) and X (angle_y)
    float cos_x = cosf(cam.angle_x), sin_x = sinf(cam.angle_x);
    float cos_y = cosf(cam.angle_y), sin_y = sinf(cam.angle_y);

    float tmp_x = rel.x * cos_x - rel.z * sin_x;
    float tmp_z = rel.x * sin_x + rel.z * cos_x;
    rel.x = tmp_x; rel.z = tmp_z;

    float tmp_y = rel.y * cos_y - rel.z * sin_y;
    tmp_z = rel.y * sin_y + rel.z * cos_y;
    rel.y = tmp_y; rel.z = tmp_z;

    if (rel.z < Z_NEAR) rel.z = Z_NEAR;

    Vec3 result;
    float inv_z = 1.0f / rel.z;

    result.x = (rel.x * focal_length * inv_z) + screen_w * 0.5f;
    result.y = (rel.y * focal_length * inv_z) + screen_h * 0.5f;

    // Normalized depth for z-buffer (0 = near, 1 = far)
    result.z = (rel.z - Z_NEAR) / (Z_FAR - Z_NEAR);

    return result;
}

static inline Vec3 project1(
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
    if (relative.z < 0.1f) relative.z = 0.00001f; 

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
    if (distance <= g_fog_start) goto done;
#ifdef g_fog_active
    if (distance >= g_fog_end) return g_fog_color;
    
    float fog_factor = (distance - g_fog_start) / (g_fog_end - g_fog_start);
    fog_factor = fmaxf(0.0f, fminf(1.0f, fog_factor));  
    
    // bit operations for faster color blending
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
#endif 
done:
    return original_color;
}

static inline uint32_t apply_lighting(
    uint32_t base_color,
    Vec3 surface_normal,
    Vec3 surface_position,
    Light light)
{
    Vec3 light_dir;
    
    if (light.is_directional) 
    {
        // directional light (like sun)
        light_dir = vec3_neg(light.direction);
    }
    else 
    {
        // point light
        light_dir = vec3_sub(light.position, surface_position);
        light_dir = vec3_normalize(light_dir);
    }
    
    // diffuse lighting calculation (Lambert's cosine law)
    float diffuse = vec3_dot(surface_normal, light_dir);
    diffuse = fmaxf(0.0f, diffuse); // positive values
    
    float ambient = 0.2f;
    float final_intensity = ambient + (diffuse * light.intensity * (1.0f - ambient));
    
    // RGB components
    uint8_t r = ((base_color >> 16) & 0xFF);
    uint8_t g = ((base_color >> 8) & 0xFF);
    uint8_t b = (base_color & 0xFF);
    
    // light intensity
    r = (uint8_t)(r * final_intensity);
    g = (uint8_t)(g * final_intensity);
    b = (uint8_t)(b * final_intensity);
    
    // color tint
    uint8_t lr = ((light.color >> 16) & 0xFF);
    uint8_t lg = ((light.color >> 8) & 0xFF);
    uint8_t lb = (light.color & 0xFF);
    
    r = (r * lr) >> 8;
    g = (g * lg) >> 8;
    b = (b * lb) >> 8;
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static inline void place_text(
        Olivec_Canvas oc, 
        char text[32]) 
{
    olivec_text(oc, text, 10, 10, olivec_default_font, 2, 0xFFFFFFFF);
}

static inline void place_triangle(
        buffer *buf, 
        Olivec_Canvas oc, 
        Vec3 tri[3],
        uint32_t c, 
        Light light, 
        Camera cam)
{ 
    if (is_back_facing(tri, cam)) return;
    
    Vec3 normal = calculate_triangle_normal(tri);

    // triangle center
    Vec3 center = {
        (tri[0].x + tri[1].x + tri[2].x) / 3.0f,
        (tri[0].y + tri[1].y + tri[2].y) / 3.0f,
        (tri[0].z + tri[1].z + tri[2].z) / 3.0f
    };
    uint32_t lit_color = apply_lighting(c, normal, center, light);

    // project vertices
    Vec3 screen[3];
    float cam_z[3];
    for (int i = 0; i < 3; i++)
    {
        screen[i] = project(tri[i], cam, buf->w, buf->h); 
        cam_z[i] = tri[i].z - cam.pos_z; // distance along Z axis
    }

    // bounding box
    int minX = fmaxf(0, floorf(fminf(screen[0].x, fminf(screen[1].x, screen[2].x))));
    int minY = fmaxf(0, floorf(fminf(screen[0].y, fminf(screen[1].y, screen[2].y))));
    int maxX = fminf(buf->w - 1, ceilf(fmaxf(screen[0].x, fmaxf(screen[1].x, screen[2].x))));
    int maxY = fminf(buf->h - 1, ceilf(fmaxf(screen[0].y, fmaxf(screen[1].y, screen[2].y))));

    // precompute denominator for barycentric coords (fast brrr)
    float denom = ((screen[1].y - screen[2].y)*(screen[0].x - screen[2].x) +
                   (screen[2].x - screen[1].x)*(screen[0].y - screen[2].y));
    if (fabsf(denom) < 1e-6f) return;

    for (int y = minY; y <= maxY; y++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            float alpha = ((screen[1].y - screen[2].y)*(x - screen[2].x) +
                           (screen[2].x - screen[1].x)*(y - screen[2].y)) / denom;
            float beta  = ((screen[2].y - screen[0].y)*(x - screen[2].x) +
                           (screen[0].x - screen[2].x)*(y - screen[2].y)) / denom;
            float gamma = 1.0f - alpha - beta;

            if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
                // Interpolate depth for z-buffer
                float z = alpha * screen[0].z + beta * screen[1].z + gamma * screen[2].z;

                // comment in if you want fog reacting to lightning (i dont)
                // float dist = alpha * cam_z[0] + beta * cam_z[1] + gamma * cam_z[2];
                float dist = distance_from_camera(center, cam);

                put_pixel_depth(buf, x, y, z, apply_fog(lit_color, dist));
            }
        }
    }
}

// static inline void place_triangle(
//         buffer *buf, 
//         Olivec_Canvas oc, 
//         Vec3 tri[3],
//         uint32_t c,
//         Light light,
//         Camera cam) 
// {
//     if (is_back_facing(tri, cam)) return;
//
//     Vec3 normal = calculate_triangle_normal(tri);
//
//     // triangle center for lighting
//     Vec3 center = {
//         (tri[0].x + tri[1].x + tri[2].x) / 3.0f,
//         (tri[0].y + tri[1].y + tri[2].y) / 3.0f,
//         (tri[0].z + tri[1].z + tri[2].z) / 3.0f
//     };
//     uint32_t lit_color = apply_lighting(c, normal, center, light);
//
//     // Early distance culling
//     float avg_distance = (
//         distance_from_camera(tri[0], cam) +
//         distance_from_camera(tri[1], cam) +
//         distance_from_camera(tri[2], cam)
//     ) / 3.0f;
//
//     if (avg_distance > g_fog_end) return; // Skip triangles beyond fog
//
//     Vec3 screen[3];
//     int visible = 0;
//     for (int i = 0; i < 3; i++) {
//         screen[i] = project(tri[i], cam, buf->w, buf->h);
//         // Check if at least one vertex is in screen bounds
//         if (screen[i].x >= -100 && screen[i].x <= buf->w + 100 &&
//             screen[i].y >= -100 && screen[i].y <= buf->h + 100 &&
//             screen[i].z > 0.1f) {
//             visible = 1;
//         }
//     }
//
//     if (!visible) return;
//
//     olivec_triangle(oc,
//             (int)screen[0].x, (int)screen[0].y,
//             (int)screen[1].x, (int)screen[1].y,
//             (int)screen[2].x, (int)screen[2].y, 
//             apply_fog(lit_color, avg_distance));
// }

static inline void place_rect_help(
        buffer *buf,
        Olivec_Canvas oc,
        Vec3 v0,
        Vec3 v1,
        Vec3 v2,
        Vec3 v3,
        uint32_t c,
        Light light,
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

    place_triangle(buf, oc, tri1, c, light, cam);
    place_triangle(buf, oc, tri2, c, light, cam);
}

static inline void place_rect(
        buffer *buf,
        Olivec_Canvas oc,
        Vec3 pos,      // origin of rectangle
        float size1,   // width along main axis
        float size2,   // height
        float angle,   // rotation in radians (around up axis)
        uint32_t c,
        RectType type,
        bool cull_other_side,
        Light light,
        Camera cam)
{
    Vec3 v0, v1, v2, v3;

    switch (type) 
    {
        case FLOOR:  // horizontal XZ
            v0 = (Vec3){0, 0, 0};
            v1 = (Vec3){size1, 0, 0};
            v2 = (Vec3){size1, 0, size2};
            v3 = (Vec3){0, 0, size2};
            break; 
        case WALL_X: // vertical YZ along X
            v0 = (Vec3){0, 0, 0};
            v1 = (Vec3){0, 0, size1};
            v2 = (Vec3){0, size2, size1};
            v3 = (Vec3){0, size2, 0};
            break;
        case WALL_Z: // vertical XY along Z
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
        verts[i].y += pos.y;  // move to world height
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


static inline void create_background(
        Olivec_Canvas oc)
{
     olivec_fill(oc, g_fog_color); 
}

static inline void create_floor(
        buffer *buf,
        Olivec_Canvas oc,
        int   tile_count_x,
        int   tile_count_z,
        float tile_size,
        float floor_y,
        Light light,
        Camera cam)
{
    for (int tz = 0; tz < tile_count_z; tz++)
    {
        for (int tx = -tile_count_x / 2; tx < tile_count_x / 2; tx++)
        {
            Vec3 pos = {
                tx * tile_size,
                floor_y,
                tz * tile_size + 4.0f - 500.0f
            };

            uint32_t color = ((tx + tz) & 1) ? 0xff000000 : 0xFFaaddff;

            place_rect(
                buf,
                oc,
                pos,               // origin corner
                tile_size,         // width (x)
                tile_size,         // depth (z)
                0.0f,              // no rotation
                color,
                FLOOR,             // orientation
                false,             // keep normal up; front face visible
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

static inline Level* level_create(int initial_capacity)
{
    Level* level = (Level*)malloc(sizeof(Level));
    level->wall_capacity = initial_capacity;
    level->wall_count = 0;
    level->walls = (Wall*)malloc(sizeof(Wall) * initial_capacity);
    return level;
}

static inline void level_add_wall(Level* level, Wall wall)
{
    if (level->wall_count >= level->wall_capacity)
    {
        level->wall_capacity *= 2;
        level->walls = (Wall*)realloc(level->walls, sizeof(Wall) * level->wall_capacity);
    }
    level->walls[level->wall_count++] = wall;
}

static inline void level_free(Level* level)
{
    free(level->walls);
    free(level);
}

static inline Level* level_load_from_file(const char* filename)
{
    FILE* f = fopen(filename, "r");
    if (!f)
    {
        printf("[ERROR] Could not open level file: %s\n", filename);
        return NULL;
    }

    Level* level = level_create(16);
    char line[256];
    int line_num = 0;

    while (fgets(line, sizeof(line), f))
    {
        line_num++;
        
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\0')
            continue;

        Wall wall = {};
        char type_str[16];
        
        // Format: x y z width height angle type [color] [flip]
        int parsed = sscanf(line, "%f %f %f %f %f %f %s %x %d",
                           &wall.pos.x, &wall.pos.y, &wall.pos.z,
                           &wall.width, &wall.height, &wall.angle,
                           type_str, &wall.color, &wall.flip_culling);

        if (parsed < 7)
        {
            printf("[WARNING] Line %d: Invalid format, skipping\n", line_num);
            continue;
        }

        // Default color if not specified
        if (parsed < 8)
            wall.color = 0xFFCCCCCC;
        
        // Default flip_culling if not specified
        if (parsed < 9)
            wall.flip_culling = 0;

        // Parse wall type
        if (strcmp(type_str, "WALL_X") == 0 || strcmp(type_str, "WALLX") == 0)
            wall.type = WALL_X;
        else if (strcmp(type_str, "WALL_Z") == 0 || strcmp(type_str, "WALLZ") == 0)
            wall.type = WALL_Z;
        else if (strcmp(type_str, "FLOOR") == 0)
            wall.type = FLOOR;
        else
        {
            printf("[WARNING] Line %d: Unknown type '%s', defaulting to WALL_X\n", 
                   line_num, type_str);
            wall.type = WALL_X;
        }

        level_add_wall(level, wall);
    }

    fclose(f);
    printf("[INFO] Loaded %d walls from %s\n", level->wall_count, filename);
    return level;
}

static inline void level_render(
    Level* level,
    buffer* buf,
    Olivec_Canvas oc,
    Light light,
    Camera cam)
{
    for (int i = 0; i < level->wall_count; i++)
    {
        Wall* w = &level->walls[i];
        place_rect(buf, oc,
                      w->pos,
                      w->width,
                      w->height,
                      w->angle,
                      w->color,
                      w->type,
                      w->flip_culling,
                      light, cam);
    }
}

#endif // UTIL_H
