#ifndef SAMPLER_H
#define SAMPLER_H

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

// Stratifies path events independently by type. Glass and diffuse counters
// reset separately for each camera sample, so an extra glass bounce cannot
// shift the dimensions used by the first diffuse bounce.
struct path_sampler {
    int samples = 1;
    int sample_index = 0;
    int grid_side = 1;
    int event_limit = 0;
    int glass_event = 0;
    int diffuse_event = 0;
    int glass_ready = 0;
    int diffuse_ready = 0;
    bool glass_enabled = false;
    bool diffuse_enabled = false;
    std::vector<std::vector<int>> glass_slots;
    std::vector<std::vector<int>> diffuse_cells;

    void begin_pixel(int sample_count, int pixel_grid_side, int max_events,
                     bool stratify_glass, bool stratify_diffuse) {
        if (sample_count <= 0 || pixel_grid_side <= 0 || max_events < 0)
            throw std::invalid_argument("path_sampler requires positive samples/grid and non-negative events");
        if (stratify_diffuse && pixel_grid_side * pixel_grid_side != sample_count)
            throw std::invalid_argument("2D diffuse stratification requires a square sample count");

        samples = sample_count;
        grid_side = pixel_grid_side;
        event_limit = max_events;
        glass_enabled = stratify_glass;
        diffuse_enabled = stratify_diffuse;
        glass_ready = 0;
        diffuse_ready = 0;
        glass_slots.resize(event_limit);
        diffuse_cells.resize(event_limit);
    }

    void begin_sample(int index) {
        if (index < 0 || index >= samples)
            throw std::out_of_range("path_sampler sample index is outside the pixel sample range");

        sample_index = index;
        glass_event = 0;
        diffuse_event = 0;
    }

    // Draw unconditionally at dielectric hits, including total internal
    // reflection, so the glass-event sequence stays aligned along each path.
    double next_glass() {
        const int event = glass_event++;
        if (!glass_enabled || event >= event_limit)
            return random_double();

        const int slot = shuffled_slot(glass_slots, glass_ready, event, samples);
        return (slot + random_double()) / samples;
    }

    // Give each diffuse bounce one jittered sample in a distinct 2D grid cell.
    // The independently permuted cell assignment avoids coupling it to the
    // camera pixel cell or to the path's glass-event count.
    std::pair<double, double> next_diffuse_pair() {
        const int event = diffuse_event++;
        if (!diffuse_enabled || event >= event_limit)
            return {random_double(), random_double()};

        const int cell = shuffled_slot(diffuse_cells, diffuse_ready, event, samples);
        const int cell_x = cell % grid_side;
        const int cell_y = cell / grid_side;
        return {
            (cell_x + random_double()) / grid_side,
            (cell_y + random_double()) / grid_side
        };
    }

    bool stratifies_diffuse() const {
        return diffuse_enabled;
    }

    vec3 random_unit_sphere_direction() {
        const auto [u, v] = next_diffuse_pair();
        const double z = 1.0 - 2.0 * u;
        const double radius = std::sqrt(1.0 - z * z);
        const double phi = 2.0 * pi * v;
        return vec3(radius * std::cos(phi), radius * std::sin(phi), z);
    }

private:
    int shuffled_slot(std::vector<std::vector<int>>& permutations,
                      int& ready, int event, int slot_count) {
        if (event == ready) {
            auto& slots = permutations[event];
            slots.resize(slot_count);
            std::iota(slots.begin(), slots.end(), 0);

            // Fisher-Yates creates an independent random permutation for this
            // event type and bounce index; do it only when the event occurs.
            for (int a = slot_count - 1; a > 0; --a)
                std::swap(slots[a], slots[int(random_double(0, a + 1))]);
            ++ready;
        }

        if (event >= ready)
            throw std::logic_error("path_sampler events must be requested in order per event type");
        return permutations[event][sample_index];
    }
};

inline thread_local path_sampler g_sampler;

#endif
