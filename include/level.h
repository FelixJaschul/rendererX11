#ifndef LEVEL_H
#define LEVEL_H

#include "util.h"
#include "triangle.h"
#include "game.h"

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
        level->walls =
            (Wall*)realloc(level->walls, sizeof(Wall) * level->wall_capacity);
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
    Level* level = level_create(16);

    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), f))
    {
        line_num++;
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\0')
            continue;

        Wall wall = {};
        char type_str[16];
        int parsed = sscanf(line, "%f %f %f %f %f %f %s %x %d",
            &wall.pos.x, &wall.pos.y, &wall.pos.z,
            &wall.width, &wall.height, &wall.angle,
            type_str, &wall.color, &wall.flip_culling);

        if (parsed < 8)
            wall.color = 0xFFCCCCCC;
        if (parsed < 9)
            wall.flip_culling = 0;

        if (strcmp(type_str, "WALL_X") == 0 ||
            strcmp(type_str, "WALLX ") == 0) wall.type = WALL_X;
        if (strcmp(type_str, "WALL_Z") == 0 ||
            strcmp(type_str, "WALLZ ") == 0) wall.type = WALL_Z;
        if (strcmp(type_str, "FLOOR") == 0) wall.type = FLOOR;

        level_add_wall(level, wall);
    }

    fclose(f);
    printf("[LOG] Loaded %d walls from %s\n", level->wall_count, filename);
    return level;
}

static inline int level_save_to_file(const Level* level, const char* filename)
{
    FILE* f = fopen(filename, "w");
    if (!f) return 0;
    for (int i = 0; i < level->wall_count; ++i)
    {
        const Wall* w = &level->walls[i];
        const char* t =
            (w->type == FLOOR) ? "FLOOR" :
            (w->type == WALL_X) ? "WALL_X" : "WALL_Z";
        // pos.x pos.y pos.z width height angle type color flip_culling
        fprintf(f, "%.6f %.6f %.6f %.6f %.6f %.6f %s %08X %d\n",
            w->pos.x, w->pos.y, w->pos.z,
            w->width, w->height, w->angle,
            t, (unsigned)w->color, w->flip_culling);
    }
    fclose(f);
    return 1;
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

#endif // LEVEL_H
