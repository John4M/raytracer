#ifndef CAMERA_H
#define CAMERA_H

#include <stdexcept>

#include "hittable.h"
#include "material.h"

class camera {
    public:
        double aspect_ratio = 1.0;
        int image_width = 100;
        int samples_per_pixel = 10;
        int max_depth = 10; //Maximum number of ray bounces into scene

        void render(const hittable& world) {
            initialize();

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
            auto start = std::chrono::steady_clock::now();

            for (int j = 0; j < image_height; j++) {
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
                for (int i = 0; i < image_width; i++) {
                    colour pixel_colour(0, 0, 0);
                    // Prepare independent glass-decision dimensions once per
                    // pixel; each camera sample then consumes its own slots.
                    g_sampler.begin_pixel(samples_per_pixel, glass_stratified_dimensions);
                    for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                        for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                            int sample_index = s_j * sqrt_spp + s_i;
                            g_sampler.begin_sample(sample_index);
                            ray r = get_ray(i, j, s_i, s_j);
                            pixel_colour += ray_colour(r, max_depth, world);
                        }
                    }
                    write_colour(std::cout, pixel_samples_scale * pixel_colour);
                }
            }

            std::clog << "\rDone.               \n";
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::clog << "Waited for: " << elapsed.count() << " seconds\n";
        }



    private:
        int image_height;
        static constexpr int glass_stratified_dimensions = 4;
        int sqrt_spp;                 // Number of grid cells along each pixel axis.
        double recip_sqrt_spp;        // Cached 1/sqrt_spp for pixel-space offsets.
        double pixel_samples_scale; //Colour scale factor for sum of pixel samples
        point3 center;
        point3 pixel00_loc;
        vec3 pixel_delta_u;
        vec3 pixel_delta_v;

        void initialize() {
            image_height = int(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            if (samples_per_pixel <= 0) {
                throw std::invalid_argument("samples_per_pixel must be positive");
            }

            // A square grid gives exactly one ray to every cell and keeps the
            // requested samples_per_pixel equal to the number actually traced.
            sqrt_spp = int(std::sqrt(samples_per_pixel));
            if (sqrt_spp * sqrt_spp != samples_per_pixel) {
                throw std::invalid_argument("samples_per_pixel must be a perfect square for stratified pixel sampling");
            }
            recip_sqrt_spp = 1.0 / sqrt_spp;
            pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);

            center = point3(0, 0, 0);

            //Viewport dimensions
            auto focal_length = 1.0;
            auto viewport_height = 2.0;
            auto viewport_width = viewport_height * (double(image_width)/image_height);

            // Calculate the vectors across the horizontal and down the vertical viewport edges.
            auto viewport_u = vec3(viewport_width, 0, 0);
            auto viewport_v = vec3(0, -viewport_height, 0);

            // Calculate the horizontal and vertical delta vectors from pixel to pixel.
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
            pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
        }

        ray get_ray(int i, int j, int s_i, int s_j) const {
            // s_i + random_double() lies in [s_i, s_i+1); scaling by 1/m and
            // centering maps that jitter into this cell within [-0.5, 0.5).
            auto px = -0.5 + recip_sqrt_spp * (s_i + random_double());
            auto py = -0.5 + recip_sqrt_spp * (s_j + random_double());
            auto pixel_sample = pixel00_loc
                                + ((i + px) * pixel_delta_u)
                                + ((j + py) * pixel_delta_v);
            
            auto ray_origin = center;
            auto ray_direction = pixel_sample - ray_origin;
            return ray(ray_origin, ray_direction); 
        }

        vec3 sample_square() const {
            //Returns vector to random position in [-0.5, -0.5]-[0.5, 0.5] unit square
            return vec3(random_double() - 0.5, random_double() - 0.5, 0);
        }

        colour ray_colour(const ray& r, int depth, const hittable& world) {
            if(depth <= 0){
                return colour(0,0,0);
            }
            
            hit_record rec;

            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                colour attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                    return attenuation * ray_colour(scattered, depth - 1, world);
                }
                
                return colour(0, 0, 0);
            }
            

            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5*(unit_direction.y() + 1.0);
            return (1.0-a)*colour(1.0, 1.0, 1.0) + a*colour(0.5, 0.7, 1.0);
        }
};

#endif