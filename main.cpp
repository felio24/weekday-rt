#include "rtweekday.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"

using namespace std;

/*double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = center - r.origin();
    auto a = r.direction().length_squared();
    auto h = dot(r.direction(), oc);
    auto c = oc.length_squared() - radius*radius;
    auto discriminant = h*h - a*c;
    
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (h - sqrt(discriminant)) / a; // we're only interested in the closest hit point (smallest t)
    }
}*/

color ray_color(const ray& r, const hittable& world) {
    hit_record rec;
    if (world.hit(r, interval(0, infinity), rec)) {
        return 0.5 * (rec.normal + color(1,1,1));
    }
    //auto t = hit_sphere(point3(0,0,-1.2), 0.5, r);
    //if (t > 0.0) { // points with negative t lie behind the camera
    //    vec3 N = unit_vector(r.at(t) - vec3(0,0,-1.2)); // P-C
    //    return 0.5*color(N.x()+1, N.y()+1, N.z()+1);
    //}

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

int main() 
{
    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    

    // World

    hittable_list world;

    world.add(make_shared<sphere>(point3(0,0,-1.2), 0.5)); // the negative z places the sphere in front of the camera, so increasing absolute z also increases the distance of the sphere from the camera
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;

    cam.render(world);
}