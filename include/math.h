#ifndef MATH_H
#define MATH_H

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


#endif // MATH_H
