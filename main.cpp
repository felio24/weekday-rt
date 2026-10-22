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

enum class scene_id {
    original,
    custom,
    new_camera
};

struct render_options {
    scene_id scene = scene_id::original;
    filter_mode filter = filter_mode::box;
    post_process_mode effect = post_process_mode::none;

    bool axes = false;
    bool dump_buffers = false;
    bool preview = false;
    bool focus_debug = false;

    int image_width = 400;
    int samples_per_pixel = 100;
    int max_depth = 50;
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

scene_config make_scene(scene_id id, const materials& mat) {
    switch (id) {
        case scene_id::original:
            return scene_original(mat);
        case scene_id::custom:
            return scene_custom(mat);
        case scene_id::new_camera:
            return scene_newcamera(mat);
    }

    throw runtime_error("Unknown scene");
}

bool parse_int(const string& text, int min_value, int max_value, int& result) {
    try {
        size_t parsed_chars = 0;
        int value = stoi(text, &parsed_chars);

        // Reject input like "400px" instead of accepting the 400
        if (parsed_chars != text.size())
            return false;

        if (value < min_value || value > max_value)
            return false;

        result = value;
        return true;
    }
    catch (const invalid_argument&) {
        return false;
    }
    catch (const out_of_range&) {
        return false;
    }
}

constexpr int min_width   = 128;
constexpr int max_width   = 2048;
constexpr int min_samples = 10;
constexpr int max_samples = 200;
constexpr int min_depth   = 10;
constexpr int max_depth   = 100;

int main(int argc, char *argv[]) {
    materials mat;
    render_options options;

    auto invalid_range_error = [](const string& flag, int min_value, int max_value) {
        return flag + " expects an integer between " +
               to_string(min_value) + " and " + to_string(max_value);
    };

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--scene") {
            if (i + 1 >= argc) {
                cerr << "--scene expects original, custom, or new-camera\n";
                return 1;
            }

            string scene_arg = argv[++i];
            if (scene_arg == "original") options.scene = scene_id::original;
            else if (scene_arg == "custom") options.scene = scene_id::custom;
            else if (scene_arg == "new-camera") options.scene = scene_id::new_camera;
            else {
                cerr << "Unknown scene: " << scene_arg << "\n";
                return 1;
            }
        } else if (arg == "--preview") {
            options.preview = true;
        } else if (arg == "--edges") {
            options.effect = post_process_mode::sobel_edge;
        } else if (arg == "--filter") {
            if (i + 1 >= argc) {
                cerr << "--filter expects box or tent\n";
                return 1;
            }

            string filter_arg = argv[++i];
            if (filter_arg == "box") options.filter = filter_mode::box;
            else if (filter_arg == "tent") options.filter = filter_mode::tent;
            else {
                cerr << "Unknown filter: " << filter_arg << "\n";
                return 1;
            }
        } else if (arg == "--axes") {
            options.axes = true;
        } else if (arg == "--debug") {
            options.focus_debug = true;
        } else if (arg == "--width") {
            if (i + 1 >= argc) {
                cerr << invalid_range_error("--width", min_width, max_width) << "\n";
                return 1;
            }

            int width;
            if (!parse_int(argv[++i], min_width, max_width, width)) {
                cerr << invalid_range_error("--width", min_width, max_width) << "\n";
                return 1;
            }

            options.image_width = width;
        } else if (arg == "--samples") {
            if (i + 1 >= argc) {
                cerr << invalid_range_error("--samples", min_samples, max_samples) << "\n";
                return 1;
            }

            int samples;
            if (!parse_int(argv[++i], min_samples, max_samples, samples)) {
                cerr << invalid_range_error("--samples", min_samples, max_samples) << "\n";
                return 1;
            }

            options.samples_per_pixel = samples;
        } else if (arg == "--depth") {
            if (i + 1 >= argc) {
                cerr << invalid_range_error("--depth", min_depth, max_depth) << "\n";
                return 1;
            }

            int depth;
            if (!parse_int(argv[++i], min_depth, max_depth, depth)) {
                cerr << invalid_range_error("--depth", min_depth, max_depth) << "\n";
                return 1;
            }

            options.max_depth = depth;
        }
    }

    scene_config scene_conf = make_scene(options.scene, mat);

    if (options.axes) {
        // Red = X, Green = Y, Blue = Z and Negative Z is into the scene
        auto x_axis_mat = make_shared<lambertian>(color(1.0, 0.1, 0.1));
        auto y_axis_mat = make_shared<lambertian>(color(0.1, 1.0, 0.1));
        auto z_axis_mat = make_shared<lambertian>(color(0.1, 0.1, 1.0));

        const double r = 0.025; // half-thickness of each axis bar
        const int axis_len = 7;

        // X axis: long in X, thin in Y and Z
        scene_conf.world.add(make_shared<box>(point3(-axis_len, -r, -r), point3(axis_len, r, r), x_axis_mat));
        // Y axis: long in Y, thin in X and Z
        scene_conf.world.add(make_shared<box>(point3(-r, -axis_len, -r), point3(r, axis_len, r), y_axis_mat));
        // Z axis: long in Z, thin in X and Y
        scene_conf.world.add(make_shared<box>(point3(-r, -r, -axis_len), point3(r, r, axis_len), z_axis_mat));
    }
    
    // Camera setup
    // Preview mode uses low sample/depth counts for fast iteration
    scene_conf.cam.aspect_ratio      = 16.0 / 9.0;
    scene_conf.cam.image_width       = options.image_width;
    scene_conf.cam.samples_per_pixel = options.preview ? 10 : options.samples_per_pixel;
    scene_conf.cam.max_depth         = options.preview ? 10 : options.max_depth;
    scene_conf.cam.effect            = options.effect;
    scene_conf.cam.filter            = options.filter;

    if (options.focus_debug) {
        auto focusPoint = scene_conf.cam.lookfrom 
                            + scene_conf.cam.focus_dist * unit_vector(scene_conf.cam.lookat - scene_conf.cam.lookfrom);

        scene_conf.world.add(make_shared<sphere>(
            focusPoint,
            0.1,
            make_shared<lambertian>(color(1,0,0))));
    }
    
    scene_conf.cam.render(scene_conf.world);
}
