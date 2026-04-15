#ifndef SURFACE_MESHER_H
#define SURFACE_MESHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "greedy_mesher.h"

typedef struct {
    int vertices[3][3];
} SurfaceMesherTriangle;

typedef struct {
    SurfaceMesherTriangle *triangles;
    size_t triangle_count;
    int cell_size;
    bool success;
} SurfaceMesherResult;

typedef struct {
    SurfaceMesherTriangle *triangles;
    size_t triangle_count;
    size_t capacity;
} SurfaceMesherTriangleBuffer;

static size_t surface_mesher_node_index(int x, int y, int z, int size_x, int size_y) {
    return (size_t)z * (size_t)size_y * (size_t)size_x + (size_t)y * (size_t)size_x + (size_t)x;
}

static bool surface_mesher_push_triangle(
    SurfaceMesherTriangleBuffer *buffer,
    const int a[3],
    const int b[3],
    const int c[3]
) {
    if (buffer->triangle_count == buffer->capacity) {
        size_t next_capacity = buffer->capacity == 0 ? 64 : buffer->capacity * 2;
        SurfaceMesherTriangle *next = (SurfaceMesherTriangle *)realloc(
            buffer->triangles,
            next_capacity * sizeof(SurfaceMesherTriangle)
        );
        if (!next) {
            return false;
        }
        buffer->triangles = next;
        buffer->capacity = next_capacity;
    }

    memcpy(buffer->triangles[buffer->triangle_count].vertices[0], a, sizeof(int) * 3);
    memcpy(buffer->triangles[buffer->triangle_count].vertices[1], b, sizeof(int) * 3);
    memcpy(buffer->triangles[buffer->triangle_count].vertices[2], c, sizeof(int) * 3);
    ++buffer->triangle_count;
    return true;
}

static void surface_mesher_midpoint(const int a[3], const int b[3], int out[3]) {
    out[0] = (a[0] + b[0]) / 2;
    out[1] = (a[1] + b[1]) / 2;
    out[2] = (a[2] + b[2]) / 2;
}

static bool surface_mesher_emit_tetra(
    SurfaceMesherTriangleBuffer *buffer,
    const int tetra_points[4][3],
    const int tetra_density[4],
    int iso_level
) {
    int inside[4];
    int outside[4];
    int inside_count = 0;
    int outside_count = 0;

    for (int i = 0; i < 4; ++i) {
        if (tetra_density[i] >= iso_level) {
            inside[inside_count++] = i;
        } else {
            outside[outside_count++] = i;
        }
    }

    if (inside_count == 0 || inside_count == 4) {
        return true;
    }

    if (inside_count == 1) {
        int p0[3], p1[3], p2[3];
        surface_mesher_midpoint(tetra_points[inside[0]], tetra_points[outside[0]], p0);
        surface_mesher_midpoint(tetra_points[inside[0]], tetra_points[outside[1]], p1);
        surface_mesher_midpoint(tetra_points[inside[0]], tetra_points[outside[2]], p2);
        return surface_mesher_push_triangle(buffer, p0, p1, p2);
    }

    if (inside_count == 3) {
        int p0[3], p1[3], p2[3];
        surface_mesher_midpoint(tetra_points[outside[0]], tetra_points[inside[0]], p0);
        surface_mesher_midpoint(tetra_points[outside[0]], tetra_points[inside[1]], p1);
        surface_mesher_midpoint(tetra_points[outside[0]], tetra_points[inside[2]], p2);
        return surface_mesher_push_triangle(buffer, p0, p2, p1);
    }

    {
        int p0[3], p1[3], p2[3], p3[3];
        surface_mesher_midpoint(tetra_points[inside[0]], tetra_points[outside[0]], p0);
        surface_mesher_midpoint(tetra_points[inside[0]], tetra_points[outside[1]], p1);
        surface_mesher_midpoint(tetra_points[inside[1]], tetra_points[outside[0]], p2);
        surface_mesher_midpoint(tetra_points[inside[1]], tetra_points[outside[1]], p3);

        if (!surface_mesher_push_triangle(buffer, p0, p1, p2)) {
            return false;
        }
        return surface_mesher_push_triangle(buffer, p1, p3, p2);
    }
}

static SurfaceMesherResult surface_mesher_build(
    const GreedyMesherPoint *points,
    size_t point_count,
    int distance,
    int resolution
) {
    SurfaceMesherResult result;
    memset(&result, 0, sizeof(result));

    {
        GreedyMesherVoxelGrid grid = greedy_mesher_voxelize(points, point_count, distance, resolution);
        if (!grid.success) {
            return result;
        }

        result.cell_size = grid.cell_size;
        if (!grid.occupied) {
            result.success = true;
            greedy_mesher_free_voxel_grid(&grid);
            return result;
        }

        {
            const int tetrahedra[6][4] = {
                {0, 5, 1, 6},
                {0, 5, 6, 4},
                {0, 2, 6, 1},
                {0, 2, 3, 6},
                {0, 7, 6, 3},
                {0, 4, 6, 7}
            };
            int node_size_x = grid.size_x + 1;
            int node_size_y = grid.size_y + 1;
            int node_size_z = grid.size_z + 1;
            size_t node_count = (size_t)node_size_x * (size_t)node_size_y * (size_t)node_size_z;
            int *density = (int *)calloc(node_count, sizeof(int));
            SurfaceMesherTriangleBuffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            if (!density) {
                greedy_mesher_free_voxel_grid(&grid);
                return result;
            }

            for (int z = 0; z < grid.size_z; ++z) {
                for (int y = 0; y < grid.size_y; ++y) {
                    for (int x = 0; x < grid.size_x; ++x) {
                        if (!greedy_mesher_is_occupied(&grid, x, y, z)) continue;
                        for (int dz = 0; dz <= 1; ++dz) {
                            for (int dy = 0; dy <= 1; ++dy) {
                                for (int dx = 0; dx <= 1; ++dx) {
                                    size_t index = surface_mesher_node_index(
                                        x + dx, y + dy, z + dz, node_size_x, node_size_y
                                    );
                                    density[index] += 1;
                                }
                            }
                        }
                    }
                }
            }

            for (int z = 0; z < grid.size_z; ++z) {
                for (int y = 0; y < grid.size_y; ++y) {
                    for (int x = 0; x < grid.size_x; ++x) {
                        int cube_points[8][3];
                        int cube_density[8];
                        int tetra_points[4][3];
                        int tetra_density[4];
                        const int corner_offsets[8][3] = {
                            {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
                        };

                        for (int i = 0; i < 8; ++i) {
                            int gx = x + corner_offsets[i][0];
                            int gy = y + corner_offsets[i][1];
                            int gz = z + corner_offsets[i][2];
                            cube_points[i][0] = (grid.min_x + gx) * grid.cell_size;
                            cube_points[i][1] = (grid.min_y + gy) * grid.cell_size;
                            cube_points[i][2] = (grid.min_z + gz) * grid.cell_size;
                            cube_density[i] = density[surface_mesher_node_index(gx, gy, gz, node_size_x, node_size_y)];
                        }

                        for (int t = 0; t < 6; ++t) {
                            for (int i = 0; i < 4; ++i) {
                                int corner = tetrahedra[t][i];
                                memcpy(tetra_points[i], cube_points[corner], sizeof(int) * 3);
                                tetra_density[i] = cube_density[corner];
                            }
                            if (!surface_mesher_emit_tetra(&buffer, tetra_points, tetra_density, 1)) {
                                free(buffer.triangles);
                                free(density);
                                greedy_mesher_free_voxel_grid(&grid);
                                return result;
                            }
                        }
                    }
                }
            }

            free(density);
            greedy_mesher_free_voxel_grid(&grid);
            result.triangles = buffer.triangles;
            result.triangle_count = buffer.triangle_count;
            result.success = true;
            return result;
        }
    }
}

static void surface_mesher_free(SurfaceMesherResult *result) {
    if (!result) return;
    free(result->triangles);
    result->triangles = NULL;
    result->triangle_count = 0;
    result->success = false;
}

#endif // SURFACE_MESHER_H
