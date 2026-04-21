#include "rtweekday.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "box.h"

using namespace std;

struct materials {
    shared_ptr<material> ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    shared_ptr<material> center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    shared_ptr<material> left   = make_shared<dielectric>(1.50);
    shared_ptr<material> bubble = make_shared<dielectric>(1.00 / 1.50);
    shared_ptr<material> right  = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
};

hittable_list scene_original(const materials& mat) {
    hittable_list scene;
    scene.add(make_shared<sphere>(point3(0.0,  0.0,    -1.2), 0.5, mat.center)); // the negative z places the sphere in front of the camera, so increasing absolute z also increases the distance of the sphere from the camera
    scene.add(make_shared<sphere>(point3(0.0,  -100.5, -1.0), 100, mat.ground));
    scene.add(make_shared<sphere>(point3(-1.0, 0.0,    -1.0), 0.5, mat.left));
    scene.add(make_shared<sphere>(point3(-1.0, 0.0,    -1.0), 0.4, mat.bubble));
    scene.add(make_shared<sphere>(point3(1.0,  0.0,    -1.0), 0.5, mat.right));
    return scene;
}

hittable_list scene_custom(const materials& mat) {
    hittable_list scene;
    scene.add(make_shared<sphere>(point3(0,     3.09,   -7.5),  3.5, mat.center));
    scene.add(make_shared<sphere>(point3(-3.55, -3.09,  -7.5),  3.5, mat.left));
    scene.add(make_shared<sphere>(point3(-3.55, -3.09,  -7.5),  3.0, mat.bubble));
    scene.add(make_shared<sphere>(point3(3.55,  -3.09,  -7.5),  3.5, mat.right));
    scene.add(make_shared<sphere>(point3(0.0,   -105.5, -10.0), 100, mat.ground));
    return scene;
}

hittable_list scene_newcamera(const materials& mat) {
    auto R = cos(pi/4);
    hittable_list scene;
    scene.add(make_shared<sphere>(point3(-R, 0, -1), R, mat.center));
    scene.add(make_shared<sphere>(point3( R, 0, -1), R, mat.right));
    return scene;
}

#define SCENE 0  // 0 = original, 1 = custom, 2 = new camera
#define AXES 0 // 0 = don't print, 1 = print axes

int main() 
{
    materials mat;
    hittable_list world;
#if SCENE == 0
    world = scene_original(mat);
#elif SCENE == 1
    world = scene_custom(mat);
#else
    world = scene_newcamera(mat);
#endif

#if AXES == 1
    auto x_axis_mat = make_shared<lambertian>(color(1.0, 0.1, 0.1)); // red
    auto y_axis_mat = make_shared<lambertian>(color(0.1, 1.0, 0.1)); // green
    auto z_axis_mat = make_shared<lambertian>(color(0.1, 0.1, 1.0)); // blue

    const double r = 0.025; // half-thickness of each axis bar

    // X axis: long in X, thin in Y and Z
    world.add(make_shared<box>(point3(-2, -r, -r), point3(2,  r,  r), x_axis_mat));

    // Y axis: long in Y, thin in X and Z
    world.add(make_shared<box>(point3(-r, -2, -r), point3( r, 2,  r), y_axis_mat));

    // Z axis: long in Z, thin in X and Y
    world.add(make_shared<box>(point3(-r, -r, -2), point3( r,  r, 2), z_axis_mat));
#endif

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 40;
    cam.lookfrom = point3(-2,2,1);
    cam.lookat   = point3(-0.2,0,-1);
    cam.vup      = vec3(0,1,0);

    cam.render(world);
}