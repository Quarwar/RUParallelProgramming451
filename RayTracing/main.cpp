/*

Parallel Programming HW05
Author: Joseph Signorile
Cite: Reference code from https://github.com/RU-ECE/ECE451-Parallel/blob/main/sessions/07/00cleaned_ray, 
ChatGPT for syntax, debugging help, and .ppm image output workaround.
HW05: Ray Tracing
GitHub: https://github.com/Quarwar/RUParallelProgramming451

Description: Using the sample ray tracer given in class, I modified the code using OpenMP to decrease
render time by splitting the work across multiple threads. The ray tracer develops 3D scenes of simple
spheres with different materials — the render time is benchmarked for the single-threaded and 
multi-threaded case.

Note: 
- Most of the code below is copied directly from the reference code in the link above. Just to save your
precious time, only look at the sections of the code that have comments: these are the sections that were actually
modified by me for multithreading.
- Unlike the original code, this code doesn't output a .webp image, but instead outputs a .ppm image. 
I had some trouble with the original libraries, so I asked ChatGPT for a workaround.
- I'm only submitting the main.cpp file, since the other files are unchanged from the reference code. 
All my files (and the reference code) are in the GitHub repo linked above; pull from there to test.

Benchmark (using testfast.conf, available in my repo):
Single-thread (original code): 24.1 s
Multi-threaded (4 threads): 1.22 s 

Commands:
g++ -O2 -fopenmp main.cpp -o main // compile
./main testfast.conf // create fast.ppm
ffmpeg -i fast.ppm fast.png // convert to png

Note: the test1.mp4 file included is the roughly 2-second output of test1.conf after 3 minutes of rendering.

*/

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <omp.h>

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

using std::make_shared;
using std::shared_ptr;
using std::string;

// Give each OpenMP thread its own generator instead of sharing rand() state.
// This is crucial step for thread safety, ensuring that each thread doesn't access the
// same rng object.
inline std::mt19937& thread_rng() {
    thread_local std::mt19937 rng([] {
        std::random_device seed;
        return std::mt19937(seed());
    }());
    return rng;
}

inline double random_double() {
    return std::uniform_real_distribution<double>(0.0, 1.0)(thread_rng());
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline double degrees_to_radians(double degrees) {
    return degrees * (pi / 180.0);
}

#include "color.h"
#include "ray.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

using namespace std;

string read_config(ifstream& conf, const string& key, string default_value) {
    string line;
    getline(conf, line);
    stringstream ss(line);
    string confkey, value;
    ss >> confkey >> value;
    if (key != confkey)
        cerr << "Key " << key << " not found in config file\n";
    return (key == confkey) ? value : default_value;
}

int read_config(ifstream& conf, const string& key, int default_value) {
    string line;
    getline(conf, line);
    stringstream ss(line);
    string confkey, value;
    ss >> confkey >> value;
    if (key != confkey)
        cerr << "Key " << key << " not found in config file\n";
    return (key == confkey) ? stoi(value) : default_value;
}

double read_config(ifstream& conf, const string& key, double default_value) {
    string line;
    getline(conf, line);
    stringstream ss(line);
    string confkey, value;
    ss >> confkey >> value;
    if (key != confkey)
        cerr << "Key " << key << " not found in config file\n";
    return (key == confkey) ? stod(value) : default_value;
}

vec3 read_config(ifstream& conf, const string& key, vec3 default_value) {
    string line;
    getline(conf, line);
    stringstream ss(line);
    string confkey;
    double x, y, z;
    ss >> confkey;
    if (key != confkey) {
        cerr << "Key " << key << " not found in config file\n";
        return default_value;
    }
    if (ss >> x >> y >> z)
        return vec3(x, y, z);
    cerr << "Could not read 3 values for " << key << "\n";
    return default_value;
}

void build_simple_world(hittable_list& world) {
    auto ground_material = make_shared<lambertian>(color(0.2, 0.2, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));
    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-1, 1, -3), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(3, 1.5, 2.5), 1.5, material3));

    auto red = color(0.65, 0.05, 0.05);
    auto albedo = red * red;
    auto sphere_material = make_shared<lambertian>(albedo);
    world.add(make_shared<sphere>(point3(4.5, 0.3, 1.0), 0.6, sphere_material));

    cout << "Built world: " << world.objects.size() << " objects\n";
}

void build_world(hittable_list& world, int min_coord, int max_coord) {
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    for (int a = min_coord; a < max_coord; a++) {
        for (int b = min_coord; b < max_coord; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;
                if (choose_mat < 0.8) {
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0,
                                  make_shared<lambertian>(color(0.4, 0.2, 0.1))));
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0,
                                  make_shared<metal>(color(0.7, 0.6, 0.5), 0.0)));
    cout << "Built world: " << world.objects.size() << " objects\n";
}

class parallel_camera {
  public:
    double aspect_ratio = 1.0;
    int image_width = 100;
    int image_height = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;
    double vfov = 90;
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3 vup = vec3(0, 1, 0);
    double defocus_angle = 0;
    double focus_dist = 10;

    void render(const hittable& world, uint32_t* pixels) {
        initialize();
        cout << "Rendering with " << omp_get_max_threads() << " OpenMP threads\n";

        // Each row writes to a separate part of the pixel buffer, so this process can be
        // done in parallel. Dynamic scheduling is used to maximize throughput, enabling
        // threads to pick up new tasks as soon as they finish.
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    pixel_color += ray_color(get_ray(i, j), max_depth, world);
                }
                pixels[j * image_width + i] = conv_color(pixel_samples_scale * pixel_color);
            }
        }
    }

  private:
    double pixel_samples_scale;
    point3 center;
    point3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    vec3 u, v, w;
    vec3 defocus_disk_u;
    vec3 defocus_disk_v;

    uint32_t conv_color(const color& pixel_color) const {
        auto r = linear_to_gamma(pixel_color.x());
        auto g = linear_to_gamma(pixel_color.y());
        auto b = linear_to_gamma(pixel_color.z());
        static const interval intensity(0.000, 0.999);
        int rbyte = int(256 * intensity.clamp(r));
        int gbyte = int(256 * intensity.clamp(g));
        int bbyte = int(256 * intensity.clamp(b));
        return 0xFF000000 | (bbyte << 16) | (gbyte << 8) | rbyte;
    }

    void initialize() {
        pixel_samples_scale = 1.0 / samples_per_pixel;
        center = lookfrom;
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);
        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    ray get_ray(int i, int j) const {
        auto offset = vec3(random_double() - 0.5, random_double() - 0.5, 0);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        return ray(ray_origin, pixel_sample - ray_origin);
    }

    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        if (depth <= 0)
            return color(0, 0, 0);
        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth - 1, world);
            return color(0, 0, 0);
        }
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }
};

FILE* setup_video_pipe(int width, int height, int fps, const char* filename) {
    char command[512];
    snprintf(command, sizeof(command),
             "ffmpeg -y -f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - "
             "-c:v libx264 -preset fast -crf 22 -pix_fmt yuv420p %s",
             width, height, fps, filename);
    return popen(command, "w");
}

void write_frame(FILE* pipe, const uint8_t* rgba_data, size_t frame_size) {
    fwrite(rgba_data, 1, frame_size, pipe);
    fflush(pipe);
}

// PPM output avoids the WebP development-library dependency of the original writer (credit: ChatGPT).
void write_ppm(const string& filename, uint32_t* pixels, int width, int height) {
    ofstream output(filename, ios::binary);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int i = 0; i < width * height; i++) {
        output.put(static_cast<char>(pixels[i] & 0xFF));
        output.put(static_cast<char>((pixels[i] >> 8) & 0xFF));
        output.put(static_cast<char>((pixels[i] >> 16) & 0xFF));
    }
}

void render(const char conffile[]) {
    ifstream conf(conffile);
    string filename = read_config(conf, "filename", "test");
    uint32_t w = read_config(conf, "w", 1200);
    uint32_t h = read_config(conf, "h", 1024);
    uint32_t num_frames = read_config(conf, "num_frames", 1);
    uint32_t worldid = read_config(conf, "world", 0);
    int min_coord = read_config(conf, "min_coord", -11);
    int max_coord = read_config(conf, "max_coord", 11);
    int samples_per_pixel = read_config(conf, "samples_per_pixel", 10);
    int max_depth = read_config(conf, "max_depth", 20);
    double vfov = read_config(conf, "vfov", 20.0);
    double defocus_angle = read_config(conf, "defocus_angle", 0.1);
    double focus_dist = read_config(conf, "focus_dist", 10.0);
    vec3 lookfrom0 = read_config(conf, "lookfrom0", vec3(13, 2, 3));
    vec3 lookat0 = read_config(conf, "lookat0", vec3(0, 0, 0));
    vec3 vup0 = read_config(conf, "vup0", vec3(0, 1, 0));
    vec3 lookfrom1 = read_config(conf, "lookfrom1", vec3(13, 2, 3));
    vec3 lookat1 = read_config(conf, "lookat1", vec3(0, 0, 0));
    vec3 vup1 = read_config(conf, "vup1", vec3(0, 1, 0));

    hittable_list world;
    if (worldid == 0)
        build_simple_world(world);
    else if (worldid == 1)
        build_world(world, min_coord, max_coord);
    else {
        cerr << "Invalid world number\n";
        return;
    }

    parallel_camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = w;
    cam.image_height = h;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth = max_depth;
    cam.vfov = vfov;
    cam.lookfrom = lookfrom0;
    cam.lookat = lookat0;
    cam.vup = vup0;
    cam.defocus_angle = defocus_angle;
    cam.focus_dist = focus_dist;

    cout << left
         << setw(20) << "filename:" << filename << '\n'
         << setw(20) << "w:" << w << '\n'
         << setw(20) << "h:" << h << '\n'
         << setw(20) << "world:" << worldid << '\n'
         << setw(20) << "samples_per_pixel:" << samples_per_pixel << '\n'
         << setw(20) << "max_depth:" << max_depth << '\n';

    const float per_frame = 1.0 / num_frames;
    const vec3 delta_lookfrom = lookfrom1 - lookfrom0;
    const vec3 delta_lookat = lookat1 - lookat0;
    const vec3 delta_vup = vup1 - vup0;
    constexpr int fps = 30;
    string video_filename = filename + ".mp4";
    FILE* video_pipe = num_frames > 1
        ? setup_video_pipe(cam.image_width, cam.image_height, fps, video_filename.c_str())
        : nullptr;
    const size_t frame_size = static_cast<size_t>(cam.image_width) * cam.image_height * 4;
    uint32_t* rgba_buffer = new uint32_t[cam.image_width * cam.image_height];
    for (uint32_t frame = 0; frame < num_frames; frame++) {
        float f = frame * per_frame;
        cam.lookfrom = lookfrom0 + f * delta_lookfrom;
        cam.lookat = lookat0 + f * delta_lookat;
        cam.vup = vup0 + f * delta_vup;
        cam.render(world, rgba_buffer);
        if (frame == 0) {
            string image_filename = filename;
            size_t extension = image_filename.rfind('.');
            if (extension == string::npos)
                image_filename += ".ppm";
            else
                image_filename.replace(extension, string::npos, ".ppm");
            write_ppm(image_filename, rgba_buffer, cam.image_width, cam.image_height);
        }
        if (num_frames > 1)
            write_frame(video_pipe, reinterpret_cast<uint8_t*>(rgba_buffer), frame_size);
    }
    if (num_frames > 1)
        pclose(video_pipe);
    delete[] rgba_buffer;
}

int main(int argc, char* argv[]) {
    // Use four threads (can set to whatever number you like)
    omp_set_num_threads(4);
    if (argc < 2) {
        render("raytrace.conf");
        return 0;
    }
    for (int i = 1; i < argc; i++)
        render(argv[i]);
    return 0;
}