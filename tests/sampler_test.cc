#include "rt.h"

#include <array>
#include <cmath>
#include <iostream>

int main() {
    path_sampler sampler;
    sampler.begin_pixel(64, 8, 4, true, false);

    int bad = 0;
    double total = 0.0;
    for (int trial = 0; trial < 1000; ++trial) {
        sampler.begin_pixel(64, 8, 4, true, false);
        int reflected = 0;
        for (int sample = 0; sample < 64; ++sample) {
            sampler.begin_sample(sample);
            if (sampler.next_glass() < 0.07)
                ++reflected;
        }
        if (reflected != 4 && reflected != 5)
            ++bad;
        total += reflected;
    }

    if (bad != 0 || total / 1000.0 < 4.40 || total / 1000.0 > 4.56) {
        std::cerr << "Unexpected glass-strata counts: bad=" << bad
                  << ", mean=" << total / 1000.0 << '\n';
        return 1;
    }

    sampler.begin_pixel(64, 8, 4, true, true);
    sampler.begin_sample(0);
    const auto first_pair = sampler.next_diffuse_pair();
    const int first_cell = int(first_pair.first * 8) + 8 * int(first_pair.second * 8);
    sampler.begin_sample(0);
    sampler.next_glass();
    sampler.next_glass();
    sampler.next_glass();
    const auto pair_after_glass = sampler.next_diffuse_pair();
    const int cell_after_glass =
        int(pair_after_glass.first * 8) + 8 * int(pair_after_glass.second * 8);
    if (first_cell != cell_after_glass) {
        std::cerr << "Glass events shifted the first diffuse event's cell\n";
        return 1;
    }

    sampler.begin_pixel(64, 8, 4, true, true);
    std::array<bool, 64> seen_cells{};
    for (int sample = 0; sample < 64; ++sample) {
        sampler.begin_sample(sample);
        const auto pair = sampler.next_diffuse_pair();
        const int x = int(pair.first * 8);
        const int y = int(pair.second * 8);
        if (x < 0 || x >= 8 || y < 0 || y >= 8) {
            std::cerr << "Diffuse pair escaped the unit square\n";
            return 1;
        }
        const int cell = y * 8 + x;
        if (seen_cells[cell]) {
            std::cerr << "Diffuse pairs reused a 2D stratum\n";
            return 1;
        }
        seen_cells[cell] = true;
    }

    sampler.begin_pixel(64, 8, 4, true, true);
    sampler.begin_sample(0);
    const vec3 direction = sampler.random_unit_sphere_direction();
    if (sampler.diffuse_ready != 1 || std::abs(direction.length() - 1.0) > 1e-12) {
        std::cerr << "Expected a unit sphere direction using one diffuse event\n";
        return 1;
    }

    sampler.begin_pixel(64, 8, 4, false, false);
    sampler.begin_sample(0);
    sampler.next_glass();
    sampler.next_diffuse_pair();
    if (sampler.glass_ready != 0 || sampler.diffuse_ready != 0) {
        std::cerr << "Plain sampling unexpectedly shuffled dimensions\n";
        return 1;
    }

    return 0;
}
