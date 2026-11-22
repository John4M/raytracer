#include <iostream>
#include <stdexcept>
#include <string>

#include "rt.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

namespace {

int parse_integer(const std::string& value) {
    std::size_t parsed = 0;
    const int result = std::stoi(value, &parsed);
    if (parsed != value.size())
        throw std::invalid_argument("not a complete integer");
    return result;
}

bool parse_switch(const std::string& value) {
    if (value == "on")
        return true;
    if (value == "off")
        return false;
    throw std::invalid_argument("expected 'on' or 'off'");
}

void set_mode(camera& cam, const std::string& mode) {
    if (mode == "plain") {
        cam.stratified_pixel = false;
        cam.stratified_glass = false;
        cam.stratified_diffuse = false;
    } else if (mode == "stratified") {
        cam.stratified_pixel = true;
        cam.stratified_glass = true;
        cam.stratified_diffuse = true;
    } else {
        throw std::invalid_argument("mode must be 'plain' or 'stratified'");
    }
}

void print_usage(const char* executable) {
    std::cerr << "Usage: " << executable
              << " [samples_per_pixel [plain|stratified]]\n"
              << "   or: " << executable
              << " [--spp N] [--mode plain|stratified]"
                 " [--pixel-stratified on|off] [--glass-stratified on|off]"
                 " [--diffuse-stratified on|off]"
                 " [--linear-output FILE] [--mask-output FILE]\n";
}

bool parse_options(int argc, char* argv[], camera& cam) {
    try {
        if (argc > 1 && argv[1][0] != '-') {
            if (argc > 3) {
                print_usage(argv[0]);
                return false;
            }
            cam.samples_per_pixel = parse_integer(argv[1]);
            if (argc == 3)
                set_mode(cam, argv[2]);
            return true;
        }

        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return false;
            }
            const std::string value = argv[++i];
            if (option == "--spp") {
                cam.samples_per_pixel = parse_integer(value);
            } else if (option == "--mode") {
                set_mode(cam, value);
            } else if (option == "--pixel-stratified") {
                cam.stratified_pixel = parse_switch(value);
            } else if (option == "--glass-stratified") {
                cam.stratified_glass = parse_switch(value);
            } else if (option == "--diffuse-stratified") {
                cam.stratified_diffuse = parse_switch(value);
            } else if (option == "--linear-output") {
                cam.linear_output_path = value;
            } else if (option == "--mask-output") {
                cam.object_mask_path = value;
            } else {
                std::cerr << "Unknown option: " << option << '\n';
                print_usage(argv[0]);
                return false;
            }
        }
    } catch (const std::invalid_argument& error) {
        std::cerr << "Invalid command-line value: " << error.what() << '\n';
        print_usage(argv[0]);
        return false;
    } catch (const std::out_of_range&) {
        std::cerr << "samples_per_pixel is outside the supported integer range.\n";
        print_usage(argv[0]);
        return false;
    }
    return true;
}

}

int main(int argc, char* argv[]) {
    // World
    hittable_list world;

    auto material_ground = make_shared<lambertian>(colour(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(colour(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.00 / 1.33);
    auto material_right  = make_shared<metal>(colour(0.8, 0.6, 0.2), 1.0);

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground, object_ids::ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.2),   0.5, material_center, object_ids::matte));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left,   object_ids::glass));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right,  object_ids::metal));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 1024;
    cam.max_depth = 50;

    if (!parse_options(argc, argv, cam))
        return 2;

    try {
        cam.render(world);
    } catch (const std::exception& error) {
        std::cerr << "Render failed: " << error.what() << '\n';
        return 1;
    }
}