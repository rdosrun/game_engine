# Adding a New Shape Asset

This folder contains shape helpers that turn a primitive descriptor into triangles
that the renderer can project and draw. Current examples:

- `cylinder.h`
- `sphere.h`
- `rectangular_prism.h`
- `pyramid.h`

The usual flow is:

1. Define or reuse a primitive descriptor in `primitive_points.h`.
2. Create a new asset header in `src/assets`.
3. Build the shape as 3D vertices and triangles.
4. Rotate and translate the vertices.
5. Call renderer mesh helpers such as `draw_mesh_triangle(...)` or
   `draw_mesh_circle(...)`.
6. Include the new asset in `assets.h` and `renderer.h`.
7. Add an example call in `render_pixels(...)`.

## Header Pattern

Use this include pattern so the asset can be included directly or through
`renderer.h`:

```c
#ifndef MY_SHAPE_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define MY_SHAPE_H

#include <stdint.h>
#include "primitive_points.h"

static DirtyRect draw_my_shape_mesh(
    const MyPrimitiveShape *shape,
    const RendererFrameContext *frame,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    uint32_t color
) {
    DirtyRect dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!shape || !frame) {
        return dirty;
    }

    /* Build vertices, rotate, translate, and draw triangles here. */

    return dirty;
}

#endif // MY_SHAPE_H
#endif
```

## Building Triangles

Use local `int vertices[][3]` arrays for `x,y,z` points. A triangle passed to
the renderer should look like this:

```c
int triangle_vertices[3][3] = {
    {x0, y0, z0},
    {x1, y1, z1},
    {x2, y2, z2}
};
```

Then draw it with:

```c
dirty = union_dirty_rects(
    dirty,
    draw_mesh_triangle(triangle_vertices, frame, color)
);
```

`draw_mesh_triangle(...)` handles:

- camera transform
- perspective projection from `x,y,z` to screen `x,y`
- depth buffer writes
- backface culling
- dirty rectangle calculation

## Rotation and Translation

If your asset has a full vertex list, rotate and translate the full list before
making triangles:

```c
rotate_vertices(vertices, vertex_count, theta_x, theta_y, theta_z);
translate_vertices(vertices, vertex_count, offset_x, offset_y, offset_z);
```

If you generate each triangle independently, rotate and translate that triangle
before drawing it:

```c
rotate_vertices(triangle_vertices, 3, theta_x, theta_y, theta_z);
translate_vertices(triangle_vertices, 3, offset_x, offset_y, offset_z);
```

## Winding and Culling

Triangle vertex order matters. The renderer uses backface culling, so triangles
with the wrong winding may disappear when they should be visible, or visible
when they should be hidden.

If a face is not drawing, swap the second and third vertices:

```c
{a, b, c}
```

becomes:

```c
{a, c, b}
```

For circle fans, use `draw_mesh_circle(...)`. It has a `flip_winding` argument
for caps that face the opposite direction:

```c
draw_mesh_circle(center, radius, frame, segments,
                 offset_x, offset_y, offset_z,
                 theta_x, theta_y, theta_z,
                 false, color);
```

Use `true` for the opposite-facing cap.

## Adding the Include

After creating `my_shape.h`, add it to `assets.h`:

```c
#include "my_shape.h"
```

Also include it in `renderer.h` near the other mesh asset includes:

```c
#include "assets/my_shape.h"
```

Those includes currently live just before `render_pixels(...)`.

## Adding a Render Example

Inside `render_pixels(...)`, add a scoped example block and union its dirty
rectangle into `current_scene_dirty`:

```c
{
    static const MyPrimitiveShape example_shape = { /* fields */ };
    DirtyRect shape_dirty = draw_my_shape_mesh(
        &example_shape,
        &frame,
        0,
        0,
        0,
        theta_x,
        theta_y,
        theta_z,
        0x00FFFFFF
    );
    current_scene_dirty = union_dirty_rects(current_scene_dirty, shape_dirty);
}
```

The `offset_x`, `offset_y`, and `offset_z` values place the shape in world
space. Use different offsets to keep examples from overlapping.

## Build Check

After adding a shape, run:

```sh
./build.sh
```

If you want a direct header syntax check, use the same compiler as the Windows
build:

```sh
x86_64-w64-mingw32-gcc -I. -fsyntax-only /tmp/my_shape_include_check.c -lm
```
