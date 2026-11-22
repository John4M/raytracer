# raytracer

## Sampling experiments

Build with CMake, then render the default stratified configuration or compare
the plain and stratified modes:

```sh
cmake -S . -B build
cmake --build build
./build/inOneWeekend
./build/inOneWeekend --spp 100 --mode plain
./build/inOneWeekend --spp 100 --mode stratified
```

Individual features can be enabled or disabled for ablation measurements:

```sh
./build/inOneWeekend --spp 100 --mode plain --pixel-stratified on
./build/inOneWeekend --spp 100 --mode plain --glass-stratified on
./build/inOneWeekend --spp 100 --mode plain --diffuse-stratified on
```

Pixel or 2D diffuse stratification requires a square sample count. Glass-only
stratification and plain sampling can use any positive count. Optional
`--linear-output FILE` writes the linear RGB render as PFM; `--mask-output FILE`
writes a PGM majority primary-hit mask (`0` sky, `1` ground, `2` matte,
`3` glass, `4` metal).