#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"

#include "hittable.h"

#include "material.h"

#include<thread>

#include<functional>

#include "ThreadCordinator.h"

#include <vector>

#include "Threadpool.h"

#include <chrono>
using namespace std::chrono;

#include <fstream>



class camera {
  public:
    void inLoop(const int& i,const int& j,const hittable& world, color* pixel_color) {
        for (int sample = 0; sample < samples_per_pixel; sample++) {
            ray r = get_ray(i, j);
            *pixel_color += ray_color(r, max_depth, world);
        }
    }


    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width  = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth         = 10;   // Maximum number of ray bounces into scene
    point3 lookfrom = point3(0,0,0);   // Point camera is looking from
    point3 lookat   = point3(0,0,-1);  // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction
    int numThreads = 8;
    std::vector<std::unique_ptr<color>> multiColorHolder;
    std::vector<std::thread> threadContainer;


    double vfov = 90;  // Vertical view angle (field of view)

    void render(const hittable& world) {
        std::ofstream scanLineTimes ("ScanLineTimes.csv");         //Opening file to print info to
        initialize();
        ThreadPool pool(numThreads);
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        std::vector<std::vector<color*>*> imageHolder;

        for (int j = 0; j < image_height; j++) {
            std::vector<color*>* scanLine = new std::vector<color*>();
            imageHolder.push_back(scanLine);
            for (int i = 0; i < image_width; i++) {
                auto a = new color(0.0,0.0,0.0);
                scanLine->push_back(a);
                pool.enqueue([this,i,j,&world, a] {
                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    *a += ray_color(r, max_depth, world);
                    }
                    });
                }
        }
        const char spinner[] = {'|', '/', '-', '\\'};
        int i = 0;
        while(!pool.all_tasks_completed()) {
            i = (i + 1) % 4;
            long long q = pool.getTTC();
            std::clog << "\r" << q << "/" << (image_height*image_width) << " || Est. Scanlines Remaining = " << image_height - (q/image_width) << "  " << spinner[i] << "   " << std::flush;
            std::this_thread::sleep_for (std::chrono::milliseconds(750));
        }

        for (int i = 0; i < imageHolder.size(); i++) {
            for (int j = 0; j < imageHolder.at(i)->size(); j++) {
                // Scale the color

                // Write the scaled color
                write_color(std::cout, pixel_samples_scale * *(imageHolder.at(i)->at(j)));
                
                // Delete the dynamically allocated color object
                delete (imageHolder.at(i)->at(j));
            }
        }

    std::clog << "\rDone.                                         \n";
    }

  private:
    int    image_height;   // Rendered image height
    point3 center;         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    vec3   u, v, w;              // Camera frame basis vectors

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        // Determine viewport dimensions.
        auto focal_length = (lookfrom - lookat).length();
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - (focal_length * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }


    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                            + ((i + offset.x()) * pixel_delta_u)
                            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth-1, world);
            return color(0,0,0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

// void scoob(const int& i,const int& j,const hittable& world, color& pixel_color, camera* yes) {
//     yes->inLoop(i,j,world,pixel_color);
// }

#endif