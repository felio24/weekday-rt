#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"

#include <chrono>

#define FILTER 0 // 0 = box, 1 = tent
#define DEBUG  0 // 0 = no depth and edge output, 1 = individual depth and edge outline printed

class camera {
  public:
    double  aspect_ratio      = 1.0; // Ratio of image width over height
    int     image_width       = 100; // Rendered image width in pixel count
    int     samples_per_pixel = 10;  // Count of random samples for each pixel
    int     max_depth         = 10;  // Maximum number of ray bounces into scene

    double vfov = 90;                 // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);  // Point camera is looking from
    point3 lookat   = point3(0,0,-1); // Point camera is looking at
    vec3   vup      = vec3(0,1,0);    // Camera-relative "up" direction

    double defocus_angle = 0; // Variation angle of rays through each pixel
    double focus_dist = 10;   // Distance from camera lookfrom point to plane of perfect focus

    double depth_threshold  = 0.5; // For sobel edge detection
    double normal_threshold = 0.8; // For sobel edge detection

    void render(const hittable& world) {
        initialize();
        srand(1234);

        auto start = chrono::high_resolution_clock::now();

        geometry_pass(world);
        color_pass(world);
        post_process();

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
                
        clog << "\rDone.                 \n";
        clog << "Render time: " << elapsed.count() << " seconds\n";
    }

  private:
    int    image_height;        // Rendered image height
    double pixel_samples_scale; // Color scale factor for a sum of pixel samples
    point3 center;              // Camera center
    point3 pixel00_loc;         // Location of pixel 0, 0
    vec3   pixel_delta_u;       // Offset to pixel to the right
    vec3   pixel_delta_v;       // Offset to pixel below
    vec3   u, v, w;             // Camera frame basis vectors
    vec3   defocus_disk_u;      // Defocus disk horizontal radius
    vec3   defocus_disk_v;      // Defocus disk vertical radius

    // framebuffers
    vector<vector<double>> fb_depth;
    vector<vector<vec3>>   fb_normal;
    vector<vector<color>>  fb_color;
    vector<vector<double>> fb_edge;

    void initialize() {
        // Calculate the image height, and ensure that it's at least 1.
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // resize framebuffers
        fb_depth.assign(image_width, vector<double>(image_height, infinity)); // assume the ray hits nothing in the world
        fb_normal.assign(image_width, vector<vec3>(image_height, vec3(0,0,0)));
        fb_color.assign(image_width, vector<vec3>(image_height, color(0,0,0)));
        fb_edge.assign(image_width, vector<double>(image_height, infinity));

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        // Determine viewport dimensions.
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);
        auto viewport_height = 2 * h * focus_dist; // decreasing focus_dist zooms out the resulting image
        auto viewport_width = viewport_height * (double(image_width)/image_height);


        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        auto viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    void geometry_pass(const hittable& world) {
        // Populate depth and normal buffers
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
                ray r(center, pixel_center - center);

                hit_record rec;
                if (world.hit(r, interval(0.001, infinity), rec)) {
                    fb_depth[i][j] = rec.t;
                    fb_normal[i][j] = rec.normal;
                }
            }
        }
    }

    void color_pass(const hittable& world) {
        cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            clog << "\rScanlines remaining: " << (image_height - j) << ' ' << flush;
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0,0,0);
                color aggregated_pixel_color(0,0,0);
#if FILTER == 0
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                aggregated_pixel_color = pixel_samples_scale * pixel_color; // remember to divide by the number of samples in each pixel
#else
                double total_weight = 0.0;
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    double weight;
                    ray r = get_ray_tent(i, j, weight);
                    pixel_color += weight * ray_color(r, max_depth, world);
                    total_weight += weight;
                }
                aggregated_pixel_color = pixel_color / total_weight;
#endif
                //write_color(cout, aggregated_pixel_color);
                fb_color[i][j] = aggregated_pixel_color;
            }
        }
    }

    void post_process() {
        compute_sobel_edges();
    }

    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j.

        auto offset = sample_square();
        // auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin; // P-C

        return ray(ray_origin, ray_direction);
    }

    ray get_ray_tent(int i, int j, double& weight) {
        auto offset = sample_square();
        
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin; // P-C

        // tent weight: separable product of two 1D tent functions
        weight = (1.0 - abs(offset.x())) * (1.0 - abs(offset.y()));
        return ray(ray_origin, ray_direction);
    }

    template<typename T, typename Formatter>
    void dump_buffer(const string& filename, const vector<vector<T>>& buffer, Formatter formatter) const {
        ofstream file(filename);
        if (!file.is_open()) { clog << "Error: could not open " << filename << "\n"; return; }
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                file << formatter(buffer[i][j]);
                if (i < image_width - 1) file << "\t";
            }
            file << "\n";
        }
    }

    template<typename T, typename Formatter>
    void dump_buffer_ppm(const string& filename, const vector<vector<T>>& buffer, Formatter to_color) const {
        ofstream file(filename);
        if (!file.is_open()) { clog << "Error: could not open " << filename << "\n"; return; }
        file << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (int j = 0; j < image_height; j++)
            for (int i = 0; i < image_width; i++)
                write_color(file, to_color(buffer[i][j]));
    }

    void compute_sobel_edges(const double& threshold = 0.05) {
        auto sample_depth = [&](int x, int y) -> double {
            x = max(0, min(x, image_width - 1));
            y = max(0, min(y, image_height - 1));
            double d = fb_depth[x][y];
            return d == infinity ? 0.0 : d;
        };

        auto sample_normal = [&](int x, int y) -> vec3 {
            x = max(0, min(x, image_width - 1));
            y = max(0, min(y, image_height - 1));
            return fb_normal[x][y];
        };

        vector<vector<double>> edge_depth;
        edge_depth.assign(image_width, vector<double>(image_height, 0.0));
        vector<vector<double>> edge_normal;
        edge_normal.assign(image_width, vector<double>(image_height, 0.0));

        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                double s_depth_x = sample_depth(i+1, j-1) + 2*sample_depth(i+1, j) + sample_depth(i+1, j+1)
                                 - sample_depth(i-1, j-1) - 2*sample_depth(i-1, j) - sample_depth(i-1, j+1);
                double s_depth_y = sample_depth(i-1, j-1) + 2*sample_depth(i, j-1) + sample_depth(i+1, j-1)
                                 - sample_depth(i-1, j+1) - 2*sample_depth(i, j+1) - sample_depth(i+1, j+1);

                vec3 s_normal_x = sample_normal(i+1, j-1) + 2*sample_normal(i+1, j) + sample_normal(i+1, j+1)
                                - sample_normal(i-1, j-1) - 2*sample_normal(i-1, j) - sample_normal(i-1, j+1);
                vec3 s_normal_y = sample_normal(i-1, j-1) + 2*sample_normal(i, j-1) + sample_normal(i+1, j-1)
                                - sample_normal(i-1, j+1) - 2*sample_normal(i, j+1) - sample_normal(i+1, j+1);

                edge_depth[i][j] = sqrt(s_depth_x * s_depth_x + s_depth_y * s_depth_y);
                edge_normal[i][j] = sqrt(s_normal_x.length_squared() + s_normal_y.length_squared());

                double edge_d = edge_depth[i][j]  > depth_threshold  ? 1.0 : 0.0;
                double edge_n = edge_normal[i][j] > normal_threshold ? 1.0 : 0.0;
                fb_edge[i][j] = max(edge_d, edge_n);

                write_color(cout, (1.0 - fb_edge[i][j]) * fb_color[i][j]);
            }
        }

#if DEBUG == 1
        dump_buffer("edge_depth.txt", edge_depth, [](const double& d) {
            return to_string(d);
        });
        dump_buffer("edge_normal.txt", edge_normal, [](const double& d) {
            return to_string(d);
        });

        dump_buffer_ppm("sobel_depth.ppm",    edge_depth,  [](const double& d) { return color(d, d, d); });
        dump_buffer_ppm("sobel_normal.ppm",   edge_normal, [](const double& d) { return color(d, d, d); });
        dump_buffer_ppm("sobel_combined.ppm", fb_edge, [](const double& d) { return color(d, d, d); });
        dump_buffer_ppm("fb_depth.ppm",  fb_depth, [](const double& d) { return color(d, d, d); });
        dump_buffer_ppm("fb_normal.ppm", fb_normal, [](const vec3& n) { return 0.5 * color(n.x()+1, n.y()+1, n.z()+1); });
#endif
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        
        if (world.hit(r, interval(0.001, infinity), rec)) { // Minimum t updated to > 0.0 to solve shadow acne issue
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth-1, world); // attenuation is a vec3 instead of a double like the previous simple implementation
            return color(0,0,0);
            //vec3 direction = rec.normal + random_unit_vector(); // Lambertian distribution
            //return 0.1* ray_color(ray(rec.p, direction), depth-1, world);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif