#ifndef SAMPLER_H
#define SAMPLER_H

#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

// Supplies stratified random values for dimensions whose sample index is known
// in advance. Every dimension gets a separate permutation so it is not locked
// to the same pixel-cell index or to another sampled dimension.
struct path_sampler {
    int n = 1;
    int k = 0;
    int dim = 0;
    std::vector<std::vector<int>> perm;

    // Prepare one independently shuffled set of slots per stratified dimension.
    // Each slot is used exactly once across the n samples in this pixel.
    void begin_pixel(int samples, int dimensions) {
        if (samples <= 0 || dimensions < 0)
            throw std::invalid_argument("path_sampler requires positive samples and non-negative dimensions");

        n = samples;
        perm.resize(dimensions);
        for (auto& slots : perm) {
            slots.resize(n);
            std::iota(slots.begin(), slots.end(), 0);

            // Fisher-Yates produces a uniformly shuffled slot assignment.
            for (int a = n - 1; a > 0; --a)
                std::swap(slots[a], slots[int(random_double(0, a + 1))]);
        }
    }

    // Start a new camera sample; dimension zero is its first glass decision.
    void begin_sample(int sample_index) {
        if (sample_index < 0 || sample_index >= n)
            throw std::out_of_range("path_sampler sample index is outside the pixel sample range");

        k = sample_index;
        dim = 0;
    }

    // Return a jittered value in the assigned stratum, then fall back to plain
    // randomness once this path has used all dimensions prepared for the pixel.
    double next() {
        if (dim < int(perm.size())) {
            int slot = perm[dim++][k];
            return (slot + random_double()) / n;
        }

        return random_double();
    }
};

inline thread_local path_sampler g_sampler;

#endif
