#include "rtweekday.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

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

    // X axis - red
    for (float t = -4; t <= 4; t += 0.2) {
        world.add(make_shared<sphere>(point3(t, 0, 0), 0.05, 
            make_shared<lambertian>(color(1,0,0))));
    }
    // Y axis - green
    for (float t = -4; t <= 4; t += 0.2) {
        world.add(make_shared<sphere>(point3(0, t, 0), 0.05, 
            make_shared<lambertian>(color(0,1,0))));
    }
    // Z axis - blue
    for (float t = -4; t <= 4; t += 0.2) {
        world.add(make_shared<sphere>(point3(0, 0, t), 0.05, 
            make_shared<lambertian>(color(0,0,1))));
    }

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