# Weekday Ray Tracer

A path tracer built from scratch in C++, following [Ray Tracing in One Weekend](https://raytracing.github.io/) and then going further. No external graphics libraries, just standard C++.

Currently adding a two-pass framebuffer system so post-processing effects can run after the ray trace, before the final image is written.

---

## Quick Start

```bash
# Build
g++ -std=c++17 main.cpp -o main

# Full quality render (slow)
./main > image.ppm

# Preview mode -- 4 samples, depth 10 (fast iteration)
./main --preview > preview.ppm

# Preview with camera focus point visualization
./main --preview --debug > debug.ppm
```

Open `.ppm` files with macOS Preview, or any image viewer that supports the format.

---

## If It Has Been Awhile...

1. Check `#define SCENE` near the top of `main.cpp` to make sure we're rendering the right scene
2. Check `#define AXES` and turn off (`0`) if the axis boxes are cluttering the view, and definitely turn off if the camera is aimed along an axis
3. Use `--preview` while tweaking; only do a full render when we're happy with the setup
4. Use `--debug` to render a small sphere at the camera's focus point when tuning depth of field
5. Check `#define FILTER` in `camera.h` -- `0` = box filter, `1` = tent filter
6. Output is PPM streamed to stdout, so always redirect: `./main > image.ppm`

---

## Coordinate System

Right-handed, Y-up:
- **X** -- right (red axis)
- **Y** -- up (green axis)
- **Z** -- toward you / out of the screen (blue axis), so the scene sits at negative Z

The colored axis boxes in the scene are controlled by `#define AXES` in `main.cpp`.

---

## Antialiasing Filters

Controlled by `#define FILTER` in `camera.h`:

- **Box filter (`0`)** -- each sample has equal weight. Simple, slightly sharp edges.
- **Tent filter (`1`)** -- samples are weighted by distance from pixel center (closer = higher weight). Gives softer, more natural-looking edges.

The tent filter uses a separable 2D tent function: `weight = (1 - |dx|) * (1 - |dy|)`, normalized by total weight at the end. The box filter is the default because it's faster and easier to reason about when debugging.

---

## Framebuffer Architecture (WIP)

The goal is a two-pass renderer: the first pass fills the framebuffers with raw render data, and the second pass runs post-processing over those buffers before writing the final image to stdout. This makes it possible to do effects like edge detection on normals or depth-based fog that aren't possible when writing pixels one at a time.

Current state:
- Three buffers are allocated in `camera::initialize()`: `fb_color`, `fb_depth`, `fb_normal`
- The render loop still writes directly to `cout` pixel by pixel -- the buffers are not being written to yet
- No post-processing pass exists yet

Next steps:
1. Modify the sample loop in `render()` to write accumulated color into `fb_color[j][i]` after the sample loop, instead of calling `write_color()` inline
2. Extend `ray_color()` to also return the depth (`rec.t`) and normal (`rec.normal`) at the first hit, and write those into `fb_depth` and `fb_normal`
3. After the render loop, add a post-processing pass that reads from the three buffers and calls `write_color()` for each pixel

---

## File Structure

```
├── main.cpp          # Scenes, shared material palette, camera config, entry point
├── camera.h          # Render loop, framebuffers, filter mode, ray_color()
├── material.h        # Lambertian, Metal, Dielectric
├── sphere.h          # Sphere primitive
├── box.h             # Axis-aligned box (used for axes visualization)
├── hittable.h        # Abstract hittable interface + hit_record
├── hittable_list.h   # Scene container
├── ray.h             # Ray struct
├── vec3.h            # Vector math + random utilities
├── color.h           # write_color(), gamma correction
├── interval.h        # Interval utility for hit range checks
└── rtweekday.h       # Shared constants (pi, infinity, etc.)
```