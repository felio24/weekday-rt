#include "rtweekday.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"

using namespace std;

hittable_list scene_original() {
    hittable_list scene;
    scene.add(make_shared<sphere>(point3(0, 0, -1.2), 0.5)); // the negative z places the sphere in front of the camera, so increasing absolute z also increases the distance of the sphere from the camera
    scene.add(make_shared<sphere>(point3(0, -100.5, -1), 100));
    return scene;
}

hittable_list scene_custom() {
    hittable_list scene;
    scene.add(make_shared<sphere>(point3(0, 0, -1.2), 0.5));
    scene.add(make_shared<sphere>(point3(0, 3.0, -7.5), 5));
    scene.add(make_shared<sphere>(point3(-3.0, -3.0, -7.5), 5));
    scene.add(make_shared<sphere>(point3(3.0, -3.0, -7.5), 5));
    return scene;
}

#define SCENE 0   // 0 = original, 1 = custom

int main() 
{
    hittable_list world;
#if SCENE == 0
    world = scene_original();
#else
    world = scene_custom();
#endif

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;

    cam.render(world);
}