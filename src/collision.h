#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

typedef struct CollisionAabb {
    float min_x;
    float min_y;
    float min_z;
    float max_x;
    float max_y;
    float max_z;
} CollisionAabb;

static bool collision_point_in_aabb(float x, float y, float z, CollisionAabb box) {
    return x >= box.min_x && x <= box.max_x &&
           y >= box.min_y && y <= box.max_y &&
           z >= box.min_z && z <= box.max_z;
}

static CollisionAabb collision_expand_aabb(CollisionAabb box, float radius, float height) {
    CollisionAabb expanded = {
        box.min_x - radius,
        box.min_y - height,
        box.min_z - radius,
        box.max_x + radius,
        box.max_y + height,
        box.max_z + radius
    };
    return expanded;
}

static bool collision_point_hits_scene(float x, float y, float z) {
    static const CollisionAabb scene_boxes[] = {
        {120.0f, -120.0f, 780.0f, 360.0f, 120.0f, 1020.0f},
        {360.0f, -120.0f, 780.0f, 600.0f, 120.0f, 1020.0f},
        {600.0f, -120.0f, 780.0f, 840.0f, 120.0f, 1020.0f},
        {840.0f, -120.0f, 780.0f, 1080.0f, 120.0f, 1020.0f},
        {1080.0f, -120.0f, 780.0f, 1320.0f, 120.0f, 1020.0f}
    };
    const float player_radius = 24.0f;
    const float player_height = 60.0f;
    const int scene_box_count = (int)(sizeof(scene_boxes) / sizeof(scene_boxes[0]));

    for (int i = 0; i < scene_box_count; ++i) {
        CollisionAabb expanded = collision_expand_aabb(scene_boxes[i], player_radius, player_height);
        if (collision_point_in_aabb(x, y, z, expanded)) {
            return true;
        }
    }

    return false;
}

static bool collision_point_hits_ground(float y) {
    const float player_height = 60.0f;
    const float ground_y = -160.0f;

    return y - player_height <= ground_y;
}

#endif // COLLISION_H
