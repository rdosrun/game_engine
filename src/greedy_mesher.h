#ifndef GREEDY_MESHER_H
#define GREEDY_MESHER_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
How to use:
1. Build an array of points that describes the object you want to approximate.
2. Call greedy_mesher_build(points, point_count, distance, resolution).
3. Iterate over result.cubes and issue one cube draw command per returned cuboid.
4. Call greedy_mesher_free(&result) when finished.

Parameter meaning:
- distance: usually the object-to-camera distance
- resolution: higher values preserve more detail
- effective cell size: max(1, distance / resolution)

Example:
    GreedyMesherPoint points[] = {
        {0, 0, 0}, {32, 0, 0}, {64, 0, 0},
        {0, 32, 0}, {32, 32, 0}, {64, 32, 0}
    };
    GreedyMesherResult mesh = greedy_mesher_build(points, 6, 300, 30);
    if (mesh.success) {
        for (size_t i = 0; i < mesh.cube_count; ++i) {
            GreedyMesherCube cube = mesh.cubes[i];
            // draw_box(cube.x, cube.y, cube.z, cube.width, cube.height, cube.depth);
        }
        greedy_mesher_free(&mesh);
    }
*/

typedef struct {
    int x;
    int y;
    int z;
} GreedyMesherPoint;

typedef struct {
    int x;
    int y;
    int z;
    int width;
    int height;
    int depth;
} GreedyMesherCube;

typedef struct {
    int x;
    int y;
    int z;
    int du_x;
    int du_y;
    int du_z;
    int dv_x;
    int dv_y;
    int dv_z;
    int axis;
    int normal_sign;
} GreedyMesherFace;

typedef struct {
    GreedyMesherCube *cubes;
    size_t cube_count;
    int cell_size;
    int min_voxel_x;
    int min_voxel_y;
    int min_voxel_z;
    int size_x;
    int size_y;
    int size_z;
    bool success;
} GreedyMesherResult;

typedef struct {
    GreedyMesherFace *faces;
    size_t face_count;
    int cell_size;
    int min_voxel_x;
    int min_voxel_y;
    int min_voxel_z;
    int size_x;
    int size_y;
    int size_z;
    bool success;
} GreedyMesherFaceResult;

/* Helper used to map arbitrary world-space points onto the voxel grid. */
static int greedy_mesher_floor_div(int value, int divisor) {
    int quotient = value / divisor;
    int remainder = value % divisor;
    if (remainder != 0 && ((remainder > 0) != (divisor > 0))) {
        --quotient;
    }
    return quotient;
}

static int greedy_mesher_cell_size(int distance, int resolution) {
    if (resolution <= 0) resolution = 1;
    if (distance < 0) distance = -distance;

    {
        int cell_size = distance / resolution;
        if (cell_size < 1) cell_size = 1;
        return cell_size;
    }
}

static int greedy_mesher_compare_points(const void *lhs, const void *rhs) {
    const GreedyMesherPoint *a = (const GreedyMesherPoint *)lhs;
    const GreedyMesherPoint *b = (const GreedyMesherPoint *)rhs;

    if (a->z != b->z) return (a->z < b->z) ? -1 : 1;
    if (a->y != b->y) return (a->y < b->y) ? -1 : 1;
    if (a->x != b->x) return (a->x < b->x) ? -1 : 1;
    return 0;
}

static size_t greedy_mesher_grid_index(int x, int y, int z, int size_x, int size_y) {
    return (size_t)z * (size_t)size_y * (size_t)size_x + (size_t)y * (size_t)size_x + (size_t)x;
}

static bool greedy_mesher_region_is_full(
    const uint8_t *occupied,
    const uint8_t *used,
    int start_x,
    int start_y,
    int start_z,
    int end_x,
    int end_y,
    int end_z,
    int size_x,
    int size_y
) {
    for (int z = start_z; z < end_z; ++z) {
        for (int y = start_y; y < end_y; ++y) {
            for (int x = start_x; x < end_x; ++x) {
                size_t index = greedy_mesher_grid_index(x, y, z, size_x, size_y);
                if (!occupied[index] || used[index]) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void greedy_mesher_mark_region_used(
    uint8_t *used,
    int start_x,
    int start_y,
    int start_z,
    int end_x,
    int end_y,
    int end_z,
    int size_x,
    int size_y
) {
    for (int z = start_z; z < end_z; ++z) {
        for (int y = start_y; y < end_y; ++y) {
            for (int x = start_x; x < end_x; ++x) {
                used[greedy_mesher_grid_index(x, y, z, size_x, size_y)] = 1;
            }
        }
    }
}

/* Public entry point. Produces merged cuboids from the input point set. */
static GreedyMesherResult greedy_mesher_build(
    const GreedyMesherPoint *points,
    size_t point_count,
    int distance,
    int resolution
) {
    GreedyMesherResult result;
    memset(&result, 0, sizeof(result));

    if (!points || point_count == 0) {
        result.success = true;
        result.cell_size = greedy_mesher_cell_size(distance, resolution);
        return result;
    }

    result.cell_size = greedy_mesher_cell_size(distance, resolution);

    GreedyMesherPoint *voxels = (GreedyMesherPoint *)malloc(point_count * sizeof(GreedyMesherPoint));
    if (!voxels) {
        return result;
    }

    for (size_t i = 0; i < point_count; ++i) {
        voxels[i].x = greedy_mesher_floor_div(points[i].x, result.cell_size);
        voxels[i].y = greedy_mesher_floor_div(points[i].y, result.cell_size);
        voxels[i].z = greedy_mesher_floor_div(points[i].z, result.cell_size);
    }

    qsort(voxels, point_count, sizeof(GreedyMesherPoint), greedy_mesher_compare_points);

    size_t voxel_count = 0;
    for (size_t i = 0; i < point_count; ++i) {
        if (voxel_count == 0 ||
            voxels[i].x != voxels[voxel_count - 1].x ||
            voxels[i].y != voxels[voxel_count - 1].y ||
            voxels[i].z != voxels[voxel_count - 1].z) {
            voxels[voxel_count++] = voxels[i];
        }
    }

    int min_x = voxels[0].x;
    int min_y = voxels[0].y;
    int min_z = voxels[0].z;
    int max_x = voxels[0].x;
    int max_y = voxels[0].y;
    int max_z = voxels[0].z;

    for (size_t i = 1; i < voxel_count; ++i) {
        if (voxels[i].x < min_x) min_x = voxels[i].x;
        if (voxels[i].y < min_y) min_y = voxels[i].y;
        if (voxels[i].z < min_z) min_z = voxels[i].z;
        if (voxels[i].x > max_x) max_x = voxels[i].x;
        if (voxels[i].y > max_y) max_y = voxels[i].y;
        if (voxels[i].z > max_z) max_z = voxels[i].z;
    }

    result.min_voxel_x = min_x;
    result.min_voxel_y = min_y;
    result.min_voxel_z = min_z;
    result.size_x = (max_x - min_x) + 1;
    result.size_y = (max_y - min_y) + 1;
    result.size_z = (max_z - min_z) + 1;

    {
        size_t grid_cells = (size_t)result.size_x * (size_t)result.size_y * (size_t)result.size_z;
        if (grid_cells == 0 || grid_cells > SIZE_MAX / sizeof(uint8_t)) {
            free(voxels);
            return result;
        }

        uint8_t *occupied = (uint8_t *)calloc(grid_cells, sizeof(uint8_t));
        uint8_t *used = (uint8_t *)calloc(grid_cells, sizeof(uint8_t));
        GreedyMesherCube *cubes = (GreedyMesherCube *)malloc(voxel_count * sizeof(GreedyMesherCube));
        if (!occupied || !used || !cubes) {
            free(voxels);
            free(occupied);
            free(used);
            free(cubes);
            return result;
        }

        for (size_t i = 0; i < voxel_count; ++i) {
            int local_x = voxels[i].x - min_x;
            int local_y = voxels[i].y - min_y;
            int local_z = voxels[i].z - min_z;
            occupied[greedy_mesher_grid_index(local_x, local_y, local_z, result.size_x, result.size_y)] = 1;
        }

        size_t cube_count = 0;
        for (int z = 0; z < result.size_z; ++z) {
            for (int y = 0; y < result.size_y; ++y) {
                for (int x = 0; x < result.size_x; ++x) {
                    size_t index = greedy_mesher_grid_index(x, y, z, result.size_x, result.size_y);
                    if (!occupied[index] || used[index]) continue;

                    int end_x = x + 1;
                    int end_y = y + 1;
                    int end_z = z + 1;

                    while (end_x < result.size_x &&
                           greedy_mesher_region_is_full(occupied, used, x, y, z, end_x + 1, y + 1, z + 1, result.size_x, result.size_y)) {
                        ++end_x;
                    }

                    while (end_y < result.size_y &&
                           greedy_mesher_region_is_full(occupied, used, x, y, z, end_x, end_y + 1, z + 1, result.size_x, result.size_y)) {
                        ++end_y;
                    }

                    while (end_z < result.size_z &&
                           greedy_mesher_region_is_full(occupied, used, x, y, z, end_x, end_y, end_z + 1, result.size_x, result.size_y)) {
                        ++end_z;
                    }

                    greedy_mesher_mark_region_used(used, x, y, z, end_x, end_y, end_z, result.size_x, result.size_y);

                    cubes[cube_count].x = (min_x + x) * result.cell_size;
                    cubes[cube_count].y = (min_y + y) * result.cell_size;
                    cubes[cube_count].z = (min_z + z) * result.cell_size;
                    cubes[cube_count].width = (end_x - x) * result.cell_size;
                    cubes[cube_count].height = (end_y - y) * result.cell_size;
                    cubes[cube_count].depth = (end_z - z) * result.cell_size;
                    ++cube_count;
                }
            }
        }

        free(voxels);
        free(occupied);
        free(used);

        result.cubes = cubes;
        result.cube_count = cube_count;
        result.success = true;
        return result;
    }
}

/* Releases memory owned by a GreedyMesherResult. */
static void greedy_mesher_free(GreedyMesherResult *result) {
    if (!result) return;
    free(result->cubes);
    result->cubes = NULL;
    result->cube_count = 0;
    result->success = false;
}

typedef struct {
    uint8_t *occupied;
    int cell_size;
    int min_x;
    int min_y;
    int min_z;
    int size_x;
    int size_y;
    int size_z;
    bool success;
} GreedyMesherVoxelGrid;

static GreedyMesherVoxelGrid greedy_mesher_voxelize(
    const GreedyMesherPoint *points,
    size_t point_count,
    int distance,
    int resolution
) {
    GreedyMesherVoxelGrid grid;
    memset(&grid, 0, sizeof(grid));
    grid.cell_size = greedy_mesher_cell_size(distance, resolution);

    if (!points || point_count == 0) {
        grid.success = true;
        return grid;
    }

    GreedyMesherPoint *voxels = (GreedyMesherPoint *)malloc(point_count * sizeof(GreedyMesherPoint));
    if (!voxels) {
        return grid;
    }

    for (size_t i = 0; i < point_count; ++i) {
        voxels[i].x = greedy_mesher_floor_div(points[i].x, grid.cell_size);
        voxels[i].y = greedy_mesher_floor_div(points[i].y, grid.cell_size);
        voxels[i].z = greedy_mesher_floor_div(points[i].z, grid.cell_size);
    }

    qsort(voxels, point_count, sizeof(GreedyMesherPoint), greedy_mesher_compare_points);

    size_t voxel_count = 0;
    for (size_t i = 0; i < point_count; ++i) {
        if (voxel_count == 0 ||
            voxels[i].x != voxels[voxel_count - 1].x ||
            voxels[i].y != voxels[voxel_count - 1].y ||
            voxels[i].z != voxels[voxel_count - 1].z) {
            voxels[voxel_count++] = voxels[i];
        }
    }

    grid.min_x = voxels[0].x;
    grid.min_y = voxels[0].y;
    grid.min_z = voxels[0].z;
    {
        int max_x = voxels[0].x;
        int max_y = voxels[0].y;
        int max_z = voxels[0].z;

        for (size_t i = 1; i < voxel_count; ++i) {
            if (voxels[i].x < grid.min_x) grid.min_x = voxels[i].x;
            if (voxels[i].y < grid.min_y) grid.min_y = voxels[i].y;
            if (voxels[i].z < grid.min_z) grid.min_z = voxels[i].z;
            if (voxels[i].x > max_x) max_x = voxels[i].x;
            if (voxels[i].y > max_y) max_y = voxels[i].y;
            if (voxels[i].z > max_z) max_z = voxels[i].z;
        }

        grid.size_x = (max_x - grid.min_x) + 1;
        grid.size_y = (max_y - grid.min_y) + 1;
        grid.size_z = (max_z - grid.min_z) + 1;
    }

    {
        size_t grid_cells = (size_t)grid.size_x * (size_t)grid.size_y * (size_t)grid.size_z;
        if (grid_cells == 0 || grid_cells > SIZE_MAX / sizeof(uint8_t)) {
            free(voxels);
            return grid;
        }

        grid.occupied = (uint8_t *)calloc(grid_cells, sizeof(uint8_t));
        if (!grid.occupied) {
            free(voxels);
            return grid;
        }

        for (size_t i = 0; i < voxel_count; ++i) {
            int local_x = voxels[i].x - grid.min_x;
            int local_y = voxels[i].y - grid.min_y;
            int local_z = voxels[i].z - grid.min_z;
            grid.occupied[greedy_mesher_grid_index(local_x, local_y, local_z, grid.size_x, grid.size_y)] = 1;
        }
    }

    free(voxels);
    grid.success = true;
    return grid;
}

static void greedy_mesher_free_voxel_grid(GreedyMesherVoxelGrid *grid) {
    if (!grid) return;
    free(grid->occupied);
    grid->occupied = NULL;
    grid->success = false;
}

static int greedy_mesher_is_occupied(const GreedyMesherVoxelGrid *grid, int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0) return 0;
    if (x >= grid->size_x || y >= grid->size_y || z >= grid->size_z) return 0;
    return grid->occupied[greedy_mesher_grid_index(x, y, z, grid->size_x, grid->size_y)] != 0;
}

static GreedyMesherFace greedy_mesher_make_face(
    int axis,
    int normal_sign,
    int plane,
    int u_start,
    int v_start,
    int width,
    int height,
    const GreedyMesherVoxelGrid *grid
) {
    int axes[3] = {0, 1, 2};
    int u = axes[(axis + 1) % 3];
    int v = axes[(axis + 2) % 3];
    int origin[3] = {0, 0, 0};
    GreedyMesherFace face;
    memset(&face, 0, sizeof(face));

    origin[axis] = plane;
    origin[u] = u_start;
    origin[v] = v_start;

    face.x = (grid->min_x + origin[0]) * grid->cell_size;
    face.y = (grid->min_y + origin[1]) * grid->cell_size;
    face.z = (grid->min_z + origin[2]) * grid->cell_size;
    face.axis = axis;
    face.normal_sign = normal_sign;

    if (u == 0) face.du_x = width * grid->cell_size;
    if (u == 1) face.du_y = width * grid->cell_size;
    if (u == 2) face.du_z = width * grid->cell_size;

    if (v == 0) face.dv_x = height * grid->cell_size;
    if (v == 1) face.dv_y = height * grid->cell_size;
    if (v == 2) face.dv_z = height * grid->cell_size;

    return face;
}

static GreedyMesherFaceResult greedy_mesher_build_faces(
    const GreedyMesherPoint *points,
    size_t point_count,
    int distance,
    int resolution
) {
    GreedyMesherFaceResult result;
    memset(&result, 0, sizeof(result));

    {
        GreedyMesherVoxelGrid grid = greedy_mesher_voxelize(points, point_count, distance, resolution);
        if (!grid.success) {
            return result;
        }

        result.cell_size = grid.cell_size;
        result.min_voxel_x = grid.min_x;
        result.min_voxel_y = grid.min_y;
        result.min_voxel_z = grid.min_z;
        result.size_x = grid.size_x;
        result.size_y = grid.size_y;
        result.size_z = grid.size_z;

        if (!grid.occupied) {
            result.success = true;
            greedy_mesher_free_voxel_grid(&grid);
            return result;
        }

        {
            int dims[3] = {grid.size_x, grid.size_y, grid.size_z};
            size_t max_faces = 6u * (size_t)grid.size_x * (size_t)grid.size_y * (size_t)grid.size_z;
            GreedyMesherFace *faces = (GreedyMesherFace *)malloc(max_faces * sizeof(GreedyMesherFace));
            int *mask_sign = (int *)malloc((size_t)((dims[0] > dims[1] ? dims[0] : dims[1]) * (dims[1] > dims[2] ? dims[1] : dims[2])) * sizeof(int));
            int *mask_used = (int *)malloc((size_t)((dims[0] > dims[1] ? dims[0] : dims[1]) * (dims[1] > dims[2] ? dims[1] : dims[2])) * sizeof(int));
            if (!faces || !mask_sign || !mask_used) {
                free(faces);
                free(mask_sign);
                free(mask_used);
                greedy_mesher_free_voxel_grid(&grid);
                return result;
            }

            size_t face_count = 0;
            for (int axis = 0; axis < 3; ++axis) {
                int u = (axis + 1) % 3;
                int v = (axis + 2) % 3;
                int mask_w = dims[u];
                int mask_h = dims[v];

                for (int plane = 0; plane <= dims[axis]; ++plane) {
                    int mask_len = mask_w * mask_h;
                    for (int i = 0; i < mask_len; ++i) {
                        mask_sign[i] = 0;
                        mask_used[i] = 0;
                    }

                    for (int vv = 0; vv < mask_h; ++vv) {
                        for (int uu = 0; uu < mask_w; ++uu) {
                            int a_coords[3] = {0, 0, 0};
                            int b_coords[3] = {0, 0, 0};
                            int index = vv * mask_w + uu;

                            a_coords[axis] = plane - 1;
                            b_coords[axis] = plane;
                            a_coords[u] = uu;
                            b_coords[u] = uu;
                            a_coords[v] = vv;
                            b_coords[v] = vv;

                            {
                                int a = greedy_mesher_is_occupied(&grid, a_coords[0], a_coords[1], a_coords[2]);
                                int b = greedy_mesher_is_occupied(&grid, b_coords[0], b_coords[1], b_coords[2]);
                                if (a != b) {
                                    mask_sign[index] = a ? 1 : -1;
                                }
                            }
                        }
                    }

                    for (int vv = 0; vv < mask_h; ++vv) {
                        for (int uu = 0; uu < mask_w; ++uu) {
                            int index = vv * mask_w + uu;
                            int sign = mask_sign[index];
                            if (sign == 0 || mask_used[index]) continue;

                            int width = 1;
                            int height = 1;

                            while (uu + width < mask_w &&
                                   mask_sign[vv * mask_w + uu + width] == sign &&
                                   !mask_used[vv * mask_w + uu + width]) {
                                ++width;
                            }

                            {
                                bool can_grow = true;
                                while (vv + height < mask_h && can_grow) {
                                    for (int k = 0; k < width; ++k) {
                                        int next_index = (vv + height) * mask_w + uu + k;
                                        if (mask_sign[next_index] != sign || mask_used[next_index]) {
                                            can_grow = false;
                                            break;
                                        }
                                    }
                                    if (can_grow) {
                                        ++height;
                                    }
                                }
                            }

                            for (int dy = 0; dy < height; ++dy) {
                                for (int dx = 0; dx < width; ++dx) {
                                    mask_used[(vv + dy) * mask_w + uu + dx] = 1;
                                }
                            }

                            faces[face_count++] = greedy_mesher_make_face(
                                axis, sign, plane, uu, vv, width, height, &grid
                            );
                        }
                    }
                }
            }

            free(mask_sign);
            free(mask_used);
            greedy_mesher_free_voxel_grid(&grid);

            result.faces = faces;
            result.face_count = face_count;
            result.success = true;
            return result;
        }
    }
}

static void greedy_mesher_free_faces(GreedyMesherFaceResult *result) {
    if (!result) return;
    free(result->faces);
    result->faces = NULL;
    result->face_count = 0;
    result->success = false;
}

#endif // GREEDY_MESHER_H
