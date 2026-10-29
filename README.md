# Weekday Ray Tracer

A compact path tracer written from scratch in C++17. It renders scenes of spheres and boxes with diffuse, metallic, and dielectric materials, depth of field, selectable sampling filters, and an optional Sobel edge pass.

The project began by following [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html), which provided the foundation for the ray, camera, geometry, and material code. It now extends that foundation with runtime scene controls, framebuffers, multiple sampling methods, and image post-processing.

## Build Requirements

- A C++17 compiler
- `make` (optional)
- An image viewer that supports PPM files

## Quick Start

Build the renderer:

```bash
# Build with make
make

# Or without
g++ -std=c++17 -02 main.cpp -o main

# Full quality render
./main > image.ppm

# Preview mode for faster iteration
./main --preview > preview.ppm
```

Open the resulting ppm file in an image viewer that supports PPM, e.g. `magick`. The renderer writes an ASCII PPM (`P3`) image to standard output, while progress and timing are written to the terminal.

Clean the local build with:

```bash
make clean
```

## How the Renderer Works

Rendering is split into three stages:

1. **Geometry pass** casts one camera ray per pixel and stores the first hit's depth and surface normal
2. **Color pass** casts jittered rays per pixel, recursively evaluates material scattering, and stores the accumulated color
3. **Post-processing pass** (optional, enabled with `--edges`) detects and blurs edges, composites the edge mask with the color buffer, and writes the final image

The framebuffer pipeline separates ray tracing from image processing and makes neighbouring-pixel effects such as edge detection possible.

## Command-Line Options

| Option | Values | Purpose |
| --- | --- | --- |
| `--scene` | `original`, `custom`, `new-camera` | Selects a scene at runtime |
| `--preview` | | Uses 10 samples and 10 ray bounces for faster render |
| `--width` | Any value between 128-2048 | Sets output image width |
| `--samples` | Any value between 10-200 | Sets the number of samples taken per pixel |
| `--depth` | Any value between 10-100 | Sets maximum ray bounce depth |
| `--filter` | `box`, `tent` | Chooses the sampling filter |
| `--edges` | | Enables Sobel outline post-processing |

Example:

```bash
./main --scene custom --width 800 --samples 150 --depth 50 --filter tent > custom.ppm
```

### Scene comparison

<table>
<tr>
<td><code>original</code></td>
<td><code>custom</code></td>
<td><code>new-camera</code></td>
</tr>
<tr>
<td><img src="docs/original.png" alt="Original scene render" width="400"></td>
<td><img src="docs/custom.png" alt="Custom scene render" width="400"></td>
<td><img src="docs/new-camera.png" alt="New-camera scene render" width="400"></td>
</tr>
<tr>
<td valign="top">A simple arrangement of diffuse, glass, and metal spheres</td>
<td valign="top">A larger composition for camera and depth-of-field experiments</td>
<td valign="top">A minimal two-sphere scene for isolating camera changes</td>
</tr>
</table>

### Preview usage

Preview mode is several times faster than a full-quality render. The exact speed gain varies with hardware, scene, and settings.

<table>
<tr>
<td><code>--preview</code></td><td>Full quality render (default)</td>
</tr>
<tr>
<td><img src="docs/preview.png" alt="Preview render for faster render" width="400"></td>
<td><img src="docs/original.png" alt="Full quality render" width="400"></td>
</tr>
<tr>
<td valign="top">10 samples per pixel, 10 ray bounces</td>
<td valign="top">100 samples per pixel, 50 ray bounces</td>
</tr>
</table>

If `--preview` is combined with `--samples` and/or `--depth`, `--preview` wins.

### Filter comparison

Each animation below alternates between <code>box</code>- and <code>tent</code>-filtered versions of the same PPM renders. The crops show different regions of the scene; only the sampling filter changes between frames.

<table>
<tr>
<td><img src="docs/box-vs-tent.gif" alt="Animated box versus tent filter comparison" width="400"></td>
<td><img src="docs/box-vs-tent-right.gif" alt="Animated right-side box versus tent filter comparison" width="400"></td>
</tr>
<tr>
<td valign="top">Original scene with default flags</td>
<td valign="top">A zoomed view of the high-contrast sphere boundary</td>
</tr>
</table>

The box filter gives every sample equal weight while the tent filter gives samples closer to the pixel centre more weight, which can soften some pixel-level variation.

### Sobel outline post-processing

<table>
<tr>
<td width="50" valign="top"><code>--edges</code> adds outlines to the render. The Sobel filter runs on the depth and surface-normal buffers from the geometry pass, and picks up sudden depth and normal changes rather than color changes in the textures. The resulting mask is blurred and composited over the rendered color.</td>
<td width="50%" ><img src="docs/raw-vs-edges.gif" alt="Animated comparison of raw rendering and edge post-processing" width="100%"></td>
</tr>
</table>

## Project Layout

```text
├── main.cpp          Scene definitions, materials, command-line parsing, entry point
├── camera.h          Camera setup, render passes, framebuffers, sampling, post-processing
├── material.h        Lambertian, metal, and dielectric materials
├── sphere.h          Sphere primitive and intersection logic
├── box.h             Axis-aligned box primitive
├── hittable.h        Hittable interface and hit records
├── hittable_list.h   Collection of scene objects
├── ray.h             Ray representation
├── vec3.h            Vector, point, and random utility functions
├── color.h           Color output and gamma correction
├── interval.h        Interval utility for hit-range checks
├── rtweekday.h       Shared constants and standard-library setup
├── docs/             Rendered scenes and visual comparisons
└── Makefile          C++17 build targets
```

## Development Roadmap

Known issues:
- Axis-box diagnostics are still experimental and can fail for rays parallel to, or starting inside the primitive
- Dielectric reflection/refraction behaviour needs to be reviewed
- Geometry buffers use a center ray while color rendering may use jittered or defocused rays
- Unresolved compiler warnings

Active or likely next improvements:
- Introduce DearImGui interface for interactive scene, camera, render-quality, filter, and post-processing controls
- Expose camera overrides and Sobel outline post-processing threshold through runtime options
- Add more primitives, lighting controls, and post-process effects
- Expand the `--debug` focus-point visualisation to support user-defined camera and scene setup

## References

- [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html)
