#ifndef PRIMITIVE_POINTS_H
#define PRIMITIVE_POINTS_H

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include "../greedy_mesher.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
Primitive shape assets for surface_mesher_build(...) or greedy_mesher_build(...).

The assets below are now shape descriptors instead of hand-written point arrays.
Call primitive_shape_generate_points(...) to produce a GreedyMesherPoint cloud,
then feed the generated points into the mesher.

Example:
    PrimitiveGeneratedPoints points = primitive_shape_generate_points(&g_cylinder_shape);
    SurfaceMesherResult mesh = surface_mesher_build(points.points, points.point_count, 900, 50);
    primitive_generated_points_free(&points);
*/

typedef enum {
    PRIMITIVE_SHAPE_CIRCLE,
    PRIMITIVE_SHAPE_PYRAMID,
    PRIMITIVE_SHAPE_CYLINDER,
    PRIMITIVE_SHAPE_BOX,
    PRIMITIVE_SHAPE_SPHERE,
    PRIMITIVE_SHAPE_STAIR
} PrimitiveShapeType;

typedef struct {
    int center[3];
    int radius;
    int step;
} PrimitiveCircle;

typedef struct {
    int base_center[3];
    int base_width;
    int base_depth;
    int height;
    int step;
} PrimitivePyramid;

typedef struct {
    int center[3];
    int radius;
    int height;
    int step;
} PrimitiveCylinder;

typedef struct {
    int center[3];
    int width;
    int height;
    int depth;
    int step;
} PrimitiveBox;

typedef struct {
    int center[3];
    int radius;
    int step;
} PrimitiveSphere;

typedef struct {
    int origin[3];
    int step_width;
    int step_height;
    int step_depth;
    int step_count;
    int sample_step;
} PrimitiveStair;

typedef struct {
    PrimitiveShapeType type;
    union {
        PrimitiveCircle circle;
        PrimitivePyramid pyramid;
        PrimitiveCylinder cylinder;
        PrimitiveBox box;
        PrimitiveSphere sphere;
        PrimitiveStair stair;
    } shape;
} PrimitiveShapeAsset;

typedef struct {
    GreedyMesherPoint *points;
    size_t point_count;
} PrimitiveGeneratedPoints;

static const PrimitiveShapeAsset g_circle_shape = {
    PRIMITIVE_SHAPE_CIRCLE,
    {.circle = {{0, 0, 0}, 96, 32}}
};

static const PrimitiveShapeAsset g_pyramid_shape = {
    PRIMITIVE_SHAPE_PYRAMID,
    {.pyramid = {{0, 0, 0}, 192, 192, 144, 32}}
};

static const PrimitiveShapeAsset g_cylinder_shape = {
    PRIMITIVE_SHAPE_CYLINDER,
    {.cylinder = {{0, 0, 0}, 96, 192, 32}}
};

static const PrimitiveShapeAsset g_box_shape = {
    PRIMITIVE_SHAPE_BOX,
    {.box = {{0, 0, 0}, 192, 192, 192, 48}}
};

static const PrimitiveShapeAsset g_sphere_shape = {
    PRIMITIVE_SHAPE_SPHERE,
    {.sphere = {{0, 0, 0}, 96, 32}}
};

static const PrimitiveShapeAsset g_stair_shape = {
    PRIMITIVE_SHAPE_STAIR,
    {.stair = {{-96, -96, -96}, 64, 48, 64, 4, 32}}
};

static int primitive_abs(int value) {
    return value < 0 ? -value : value;
}

static int primitive_round_double(double value) {
    return (int)(value + (value >= 0.0 ? 0.5 : -0.5));
}

static int primitive_positive_step(int step) {
    return step <= 0 ? 1 : step;
}

static bool primitive_push_point(PrimitiveGeneratedPoints *generated, size_t *capacity, int x, int y, int z) {
    if (generated->point_count == *capacity) {
        size_t next_capacity = *capacity == 0 ? 64 : *capacity * 2;
        GreedyMesherPoint *next = (GreedyMesherPoint *)realloc(
            generated->points,
            next_capacity * sizeof(GreedyMesherPoint)
        );
        if (!next) {
            return false;
        }
        generated->points = next;
        *capacity = next_capacity;
    }

    generated->points[generated->point_count].x = x;
    generated->points[generated->point_count].y = y;
    generated->points[generated->point_count].z = z;
    ++generated->point_count;
    return true;
}

static bool primitive_generate_filled_box(
    PrimitiveGeneratedPoints *generated,
    size_t *capacity,
    int min_x,
    int min_y,
    int min_z,
    int max_x,
    int max_y,
    int max_z,
    int step
) {
    step = primitive_positive_step(step);
    for (int z = min_z; z <= max_z; z += step) {
        for (int y = min_y; y <= max_y; y += step) {
            for (int x = min_x; x <= max_x; x += step) {
                if (!primitive_push_point(generated, capacity, x, y, z)) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool primitive_generate_circle_points(
    const PrimitiveCircle *circle,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    int step = primitive_positive_step(circle->step);
    int r2 = circle->radius * circle->radius;

    for (int y = -circle->radius; y <= circle->radius; y += step) {
        for (int x = -circle->radius; x <= circle->radius; x += step) {
            if ((x * x) + (y * y) <= r2) {
                if (!primitive_push_point(
                    generated,
                    capacity,
                    circle->center[0] + x,
                    circle->center[1] + y,
                    circle->center[2]
                )) {
                    return false;
                }
            }
        }
    }

    return true;
}

static bool primitive_generate_pyramid_points(
    const PrimitivePyramid *pyramid,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    int step = primitive_positive_step(pyramid->step);
    int levels = pyramid->height / step;
    if (levels < 1) levels = 1;

    for (int level = 0; level <= levels; ++level) {
        int y = (level * pyramid->height) / levels;
        int width = (pyramid->base_width * (levels - level)) / levels;
        int depth = (pyramid->base_depth * (levels - level)) / levels;
        int half_width = width / 2;
        int half_depth = depth / 2;

        if (half_width < 1) half_width = 1;
        if (half_depth < 1) half_depth = 1;

        for (int z = -half_depth; z <= half_depth; z += step) {
            for (int x = -half_width; x <= half_width; x += step) {
                if (!primitive_push_point(
                    generated,
                    capacity,
                    pyramid->base_center[0] + x,
                    pyramid->base_center[1] + y,
                    pyramid->base_center[2] + z
                )) {
                    return false;
                }
            }
        }
    }

    return true;
}

static bool primitive_generate_cylinder_points(
    const PrimitiveCylinder *cylinder,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    int step = primitive_positive_step(cylinder->step);
    int r2 = cylinder->radius * cylinder->radius;
    int half_height = cylinder->height / 2;

    for (int y = -half_height; y <= half_height; y += step) {
        for (int z = -cylinder->radius; z <= cylinder->radius; z += step) {
            for (int x = -cylinder->radius; x <= cylinder->radius; x += step) {
                if ((x * x) + (z * z) <= r2) {
                    if (!primitive_push_point(
                        generated,
                        capacity,
                        cylinder->center[0] + x,
                        cylinder->center[1] + y,
                        cylinder->center[2] + z
                    )) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

static bool primitive_generate_box_points(
    const PrimitiveBox *box,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    return primitive_generate_filled_box(
        generated,
        capacity,
        box->center[0] - box->width / 2,
        box->center[1] - box->height / 2,
        box->center[2] - box->depth / 2,
        box->center[0] + box->width / 2,
        box->center[1] + box->height / 2,
        box->center[2] + box->depth / 2,
        box->step
    );
}

static bool primitive_generate_sphere_points(
    const PrimitiveSphere *sphere,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    int step = primitive_positive_step(sphere->step);
    int r2 = sphere->radius * sphere->radius;

    for (int z = -sphere->radius; z <= sphere->radius; z += step) {
        for (int y = -sphere->radius; y <= sphere->radius; y += step) {
            for (int x = -sphere->radius; x <= sphere->radius; x += step) {
                if ((x * x) + (y * y) + (z * z) <= r2) {
                    if (!primitive_push_point(
                        generated,
                        capacity,
                        sphere->center[0] + x,
                        sphere->center[1] + y,
                        sphere->center[2] + z
                    )) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

static bool primitive_generate_stair_points(
    const PrimitiveStair *stair,
    PrimitiveGeneratedPoints *generated,
    size_t *capacity
) {
    int sample_step = primitive_positive_step(stair->sample_step);

    for (int step_index = 0; step_index < stair->step_count; ++step_index) {
        int min_x = stair->origin[0];
        int max_x = stair->origin[0] + stair->step_width * (step_index + 1);
        int min_y = stair->origin[1];
        int max_y = stair->origin[1] + stair->step_height * (step_index + 1);
        int min_z = stair->origin[2] + stair->step_depth * step_index;
        int max_z = stair->origin[2] + stair->step_depth * (step_index + 1);

        if (!primitive_generate_filled_box(
            generated,
            capacity,
            min_x,
            min_y,
            min_z,
            max_x,
            max_y,
            max_z,
            sample_step
        )) {
            return false;
        }
    }

    return true;
}

static PrimitiveGeneratedPoints primitive_shape_generate_points(const PrimitiveShapeAsset *asset) {
    PrimitiveGeneratedPoints generated;
    size_t capacity = 0;
    bool success = false;
    generated.points = NULL;
    generated.point_count = 0;

    if (!asset) {
        return generated;
    }

    switch (asset->type) {
        case PRIMITIVE_SHAPE_CIRCLE:
            success = primitive_generate_circle_points(&asset->shape.circle, &generated, &capacity);
            break;
        case PRIMITIVE_SHAPE_PYRAMID:
            success = primitive_generate_pyramid_points(&asset->shape.pyramid, &generated, &capacity);
            break;
        case PRIMITIVE_SHAPE_CYLINDER:
            success = primitive_generate_cylinder_points(&asset->shape.cylinder, &generated, &capacity);
            break;
        case PRIMITIVE_SHAPE_BOX:
            success = primitive_generate_box_points(&asset->shape.box, &generated, &capacity);
            break;
        case PRIMITIVE_SHAPE_SPHERE:
            success = primitive_generate_sphere_points(&asset->shape.sphere, &generated, &capacity);
            break;
        case PRIMITIVE_SHAPE_STAIR:
            success = primitive_generate_stair_points(&asset->shape.stair, &generated, &capacity);
            break;
    }

    if (!success) {
        free(generated.points);
        generated.points = NULL;
        generated.point_count = 0;
    }

    return generated;
}

static void primitive_generated_points_free(PrimitiveGeneratedPoints *generated) {
    if (!generated) return;
    free(generated->points);
    generated->points = NULL;
    generated->point_count = 0;
}

#endif // PRIMITIVE_POINTS_H
