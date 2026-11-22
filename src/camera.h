#ifndef CAMERA_H
#define CAMERA_H

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "hittable.h"
#include "material.h"

class camera {
    public:
        double aspect_ratio = 1.0;
        int image_width = 100;
        int samples_per_pixel = 10;
        int max_depth = 10;
        bool stratified_pixel = true;
        bool stratified_glass = true;
        bool stratified_diffuse = true;

        // Optional diagnostic outputs: linear_output_path is PFM RGB float;
        // object_mask_path is a PGM mask with IDs documented by the scene.
        std::string linear_output_path;
        std::string object_mask_path;

        void render(const hittable& world) {
            initialize();

            std::vector<colour> linear_pixels;
            std::vector<int> object_mask;
            const bool capture_linear = !linear_output_path.empty();
            const bool capture_mask = !object_mask_path.empty();
            if (capture_linear)
                linear_pixels.resize(image_width * image_height);
            if (capture_mask)
                object_mask.resize(image_width * image_height);

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
            auto start = std::chrono::steady_clock::now();

            for (int j = 0; j < image_height; j++) {
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
                for (int i = 0; i < image_width; i++) {
                    colour pixel_colour(0, 0, 0);
                    std::array<int, object_id_count> object_votes{};

                    // Event-type counters are independent: glass bounces do
                    // not shift the dimensions used by diffuse bounces.
                    g_sampler.begin_pixel(samples_per_pixel, sqrt_spp, max_depth,
                                          stratified_glass, stratified_diffuse);
                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                        ray r;
                        if (stratified_pixel) {
                            const int s_i = sample % sqrt_spp;
                            const int s_j = sample / sqrt_spp;
                            r = get_stratified_ray(i, j, s_i, s_j);
                        } else {
                            r = get_ray(i, j);
                        }

                        g_sampler.begin_sample(sample);
                        if (capture_mask)
                            ++object_votes[primary_object_id(r, world)];
                        pixel_colour += ray_colour(r, max_depth, world);
                    }

                    const int pixel_index = j * image_width + i;
                    const colour averaged_colour = pixel_samples_scale * pixel_colour;
                    if (capture_linear)
                        linear_pixels[pixel_index] = averaged_colour;
                    if (capture_mask) {
                        int dominant_id = 0;
                        for (int id = 1; id < int(object_votes.size()); ++id) {
                            if (object_votes[id] > object_votes[dominant_id])
                                dominant_id = id;
                        }
                        object_mask[pixel_index] = dominant_id;
                    }
                    write_colour(std::cout, averaged_colour);
                }
            }

            std::clog << "\rDone.               \n";
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::clog << "Waited for: " << elapsed.count() << " seconds\n";

            if (capture_linear)
                write_linear_pfm(linear_pixels);
            if (capture_mask)
                write_object_mask(object_mask);
        }

    private:
        static constexpr int object_id_count = object_ids::count;
        int image_height;
        int sqrt_spp;
        double recip_sqrt_spp;
        double pixel_samples_scale;
        point3 center;
        point3 pixel00_loc;
        vec3 pixel_delta_u;
        vec3 pixel_delta_v;

        void initialize() {
            if (image_width <= 0 || !std::isfinite(aspect_ratio) || aspect_ratio <= 0.0)
                throw std::invalid_argument("image_width and aspect_ratio must be positive");
            if (samples_per_pixel <= 0)
                throw std::invalid_argument("samples_per_pixel must be positive");
            if (max_depth < 0)
                throw std::invalid_argument("max_depth cannot be negative");

            const double requested_height = image_width / aspect_ratio;
            if (!std::isfinite(requested_height) ||
                requested_height > std::numeric_limits<int>::max()) {
                throw std::invalid_argument("aspect_ratio produces an unsupported image height");
            }
            image_height = std::max(1, int(requested_height));

            sqrt_spp = int(std::sqrt(samples_per_pixel));
            const bool needs_square_samples = stratified_pixel || stratified_diffuse;
            if (needs_square_samples && sqrt_spp * sqrt_spp != samples_per_pixel) {
                throw std::invalid_argument(
                    "stratified pixel or diffuse sampling requires samples_per_pixel to be a perfect square");
            }
            recip_sqrt_spp = 1.0 / sqrt_spp;
            pixel_samples_scale = 1.0 / samples_per_pixel;

            center = point3(0, 0, 0);

            const auto focal_length = 1.0;
            const auto viewport_height = 2.0;
            const auto viewport_width = viewport_height * (double(image_width) / image_height);

            const auto viewport_u = vec3(viewport_width, 0, 0);
            const auto viewport_v = vec3(0, -viewport_height, 0);
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            const auto viewport_upper_left =
                center - vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
            pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
        }

        ray get_stratified_ray(int i, int j, int s_i, int s_j) const {
            // Jitter one sample inside the selected cell of the pixel grid.
            const auto px = -0.5 + recip_sqrt_spp * (s_i + random_double());
            const auto py = -0.5 + recip_sqrt_spp * (s_j + random_double());
            const auto pixel_sample = pixel00_loc
                                      + ((i + px) * pixel_delta_u)
                                      + ((j + py) * pixel_delta_v);
            return ray(center, pixel_sample - center);
        }

        ray get_ray(int i, int j) const {
            const auto offset = sample_square();
            const auto pixel_sample = pixel00_loc
                                      + ((i + offset.x()) * pixel_delta_u)
                                      + ((j + offset.y()) * pixel_delta_v);
            return ray(center, pixel_sample - center);
        }

        vec3 sample_square() const {
            return vec3(random_double() - 0.5, random_double() - 0.5, 0);
        }

        int primary_object_id(const ray& r, const hittable& world) const {
            hit_record rec;
            if (!world.hit(r, interval(0.001, infinity), rec))
                return 0;
            if (rec.object_id < 0 || rec.object_id >= object_id_count)
                throw std::runtime_error("hit object ID is outside the diagnostic mask range");
            return rec.object_id;
        }

        colour ray_colour(const ray& r, int depth, const hittable& world) const {
            if (depth <= 0)
                return colour(0, 0, 0);

            hit_record rec;
            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                colour attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered))
                    return attenuation * ray_colour(scattered, depth - 1, world);
                return colour(0, 0, 0);
            }

            const vec3 unit_direction = unit_vector(r.direction());
            const auto a = 0.5 * (unit_direction.y() + 1.0);
            return (1.0 - a) * colour(1.0, 1.0, 1.0) + a * colour(0.5, 0.7, 1.0);
        }

        void write_linear_pfm(const std::vector<colour>& pixels) const {
            std::ofstream out(linear_output_path, std::ios::binary);
            if (!out)
                throw std::runtime_error("cannot open linear PFM output: " + linear_output_path);

            // PFM stores rows bottom-up; -1 declares little-endian float data.
            out << "PF\n" << image_width << ' ' << image_height << "\n-1.0\n";
            for (int y = image_height - 1; y >= 0; --y) {
                for (int x = 0; x < image_width; ++x) {
                    const colour& c = pixels[y * image_width + x];
                    const float rgb[3] = {float(c.x()), float(c.y()), float(c.z())};
                    out.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
                }
            }
            if (!out)
                throw std::runtime_error("failed while writing linear PFM output: " + linear_output_path);
        }

        void write_object_mask(const std::vector<int>& mask) const {
            std::ofstream out(object_mask_path);
            if (!out)
                throw std::runtime_error("cannot open object mask output: " + object_mask_path);

            out << "P2\n" << image_width << ' ' << image_height << "\n"
                << object_id_count - 1 << '\n';
            for (int y = 0; y < image_height; ++y) {
                for (int x = 0; x < image_width; ++x) {
                    if (x != 0)
                        out << ' ';
                    out << mask[y * image_width + x];
                }
                out << '\n';
            }
            if (!out)
                throw std::runtime_error("failed while writing object mask output: " + object_mask_path);
        }
};

#endif
