#include "rtweekday.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "box.h"

using namespace std;

// Shared material palette used across all scenes
// Swap out individual materials here to affect every scene that references them
struct materials {
    shared_ptr<material> ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    shared_ptr<material> center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    shared_ptr<material> left   = make_shared<dielectric>(1.50);
    shared_ptr<material> bubble = make_shared<dielectric>(1.00 / 1.50); // air inside glass: RI = air/glass
    shared_ptr<material> right  = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
};

struct scene_config {
    hittable_list world;
    camera        cam;
};

scene_config scene_original(const materials& mat) {
    scene_config scene;
    scene.world.add(make_shared<sphere>(point3(0.0,  0.0,    -1.2), 0.5, mat.center)); // the negative z places the sphere in front of the camera, so increasing absolute z also increases the distance of the sphere from the camera
    scene.world.add(make_shared<sphere>(point3(0.0,  -100.5, -1.0), 100, mat.ground));
    scene.world.add(make_shared<sphere>(point3(-1.0, 0.0,    -1.0), 0.5, mat.left));
    scene.world.add(make_shared<sphere>(point3(-1.0, 0.0,    -1.0), 0.4, mat.bubble));
    scene.world.add(make_shared<sphere>(point3(1.0,  0.0,    -1.0), 0.5, mat.right));

    scene.cam.vfov     = 20;
    scene.cam.lookfrom = point3(-2, 2, 1);;
    scene.cam.lookat   = point3(-0.2, 0, -1);
    scene.cam.vup      = vec3(0, 1, 0);
    
    scene.cam.defocus_angle = 10.0;
    scene.cam.focus_dist    = 3.4;
    return scene;
}

// Larger scale scene -- bigger spheres, more breathing room between them
// Better for testing camera angles and depth of field
scene_config scene_custom(const materials& mat) {
    scene_config scene;
    scene.world.add(make_shared<sphere>(point3(0,     3.09,   -7.5),  3.5, mat.center));
    scene.world.add(make_shared<sphere>(point3(-3.55, -3.09,  -7.5),  3.5, mat.left));
    scene.world.add(make_shared<sphere>(point3(-3.55, -3.09,  -7.5),  3.0, mat.bubble));
    scene.world.add(make_shared<sphere>(point3(3.55,  -3.09,  -7.5),  3.5, mat.right));
    scene.world.add(make_shared<sphere>(point3(0.0,   -105.5, -10.0), 100, mat.ground));

    scene.cam.vfov     = 25;
    scene.cam.lookfrom = point3(5.5, 1.8, 5.5);
    scene.cam.lookat   = point3(2.2, 1.3, -1);
    scene.cam.vup      = vec3(0, 1, 0);
    
    scene.cam.defocus_angle = 5.0;
    scene.cam.focus_dist    = 14.1;
    return scene;
}

// Minimal two-sphere scene for isolating camera parameter changes
scene_config scene_newcamera(const materials& mat) {
    auto R = cos(pi/4);
    scene_config scene;
    scene.world.add(make_shared<sphere>(point3(-R, 0, -1), R, mat.center));
    scene.world.add(make_shared<sphere>(point3( R, 0, -1), R, mat.right));

    scene.cam.vfov     = 20;
    scene.cam.lookfrom = point3(-2, 2, 1);;
    scene.cam.lookat   = point3(-0.2, 0, -1);
    scene.cam.vup      = vec3(0, 1, 0);
    
    scene.cam.defocus_angle = 7.0;
    scene.cam.focus_dist    = 2.9;
    return scene;
}

#define SCENE 0 // 0 = original, 1 = custom, 2 = new camera
#define AXES  0 // 0 = off, 1 = on (turn off if camera is aimed along an axis)

int main(int argc, char *argv[]) 
{
    materials mat;
    scene_config scene;

#if SCENE == 0
    scene = scene_original(mat);
#elif SCENE == 1
    scene = scene_custom(mat);
#else
    scene = scene_newcamera(mat);
#endif

#if AXES == 1
    // Red = X, Green = Y, Blue = Z and Negative Z is into the scene
    auto x_axis_mat = make_shared<lambertian>(color(1.0, 0.1, 0.1));
    auto y_axis_mat = make_shared<lambertian>(color(0.1, 1.0, 0.1));
    auto z_axis_mat = make_shared<lambertian>(color(0.1, 0.1, 1.0));

    const double r = 0.025; // half-thickness of each axis bar
    const int axis_len = 7;

    // X axis: long in X, thin in Y and Z
    scene.world.add(make_shared<box>(point3(-axis_len, -r, -r), point3(axis_len, r, r), x_axis_mat));
    // Y axis: long in Y, thin in X and Z
    scene.world.add(make_shared<box>(point3(-r, -axis_len, -r), point3(r, axis_len, r), y_axis_mat));
    // Z axis: long in Z, thin in X and Y
    scene.world.add(make_shared<box>(point3(-r, -r, -axis_len), point3(r, r, axis_len), z_axis_mat));
#endif

    bool preview   = false;
    bool debugView = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--preview") preview   = true;
        if (arg == "--debug")   debugView = true;
    }


    // Camera setup
    // Preview mode uses low sample/depth counts for fast iteration
    scene.cam.aspect_ratio      = 16.0 / 9.0;
    scene.cam.image_width       = 400;
    scene.cam.samples_per_pixel = preview ? 10 : 100;
    scene.cam.max_depth         = preview ? 10 : 50;

    if (debugView)
    {
        auto focusPoint = scene.cam.lookfrom 
                            + scene.cam.focus_dist * unit_vector(scene.cam.lookat - scene.cam.lookfrom);

        scene.world.add(make_shared<sphere>(
            focusPoint,
            0.1,
            make_shared<lambertian>(color(1,0,0))));
    }
    
    scene.cam.render(scene.world);
}