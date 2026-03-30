// material_types.c - Material type implementation

#include "material_types.h"

// Material properties database
const MaterialProperties MATERIAL_TABLE[MATERIAL_COUNT] = {
    [MATERIAL_ROCK] = {
        .name = "Rock",
        .base_mass = 1.0f,
        .erosion_rate = 0.001f,
        .thermal_conductivity = 0.5f,
        .flags = FLAG_DENSE,
        .color_r = 139, .color_g = 69, .color_b = 19
    },
    [MATERIAL_ICE] = {
        .name = "Ice",
        .base_mass = 0.9f,
        .erosion_rate = 0.01f,
        .thermal_conductivity = 2.0f,
        .flags = FLAG_BUOYANT,
        .color_r = 173, .color_g = 216, .color_b = 230
    },
    [MATERIAL_METAL] = {
        .name = "Metal",
        .base_mass = 2.5f,
        .erosion_rate = 0.0001f,
        .thermal_conductivity = 5.0f,
        .flags = FLAG_DENSE | FLAG_REFLECTIVE | FLAG_MAGNETIC,
        .color_r = 192, .color_g = 192, .color_b = 192
    },
    [MATERIAL_GAS] = {
        .name = "Gas",
        .base_mass = 0.01f,
        .erosion_rate = 0.0f,
        .thermal_conductivity = 0.1f,
        .flags = FLAG_BUOYANT | FLAG_FUEL,
        .color_r = 147, .color_g = 112, .color_b = 219
    },
    [MATERIAL_ORGANIC] = {
        .name = "Organic",
        .base_mass = 0.8f,
        .erosion_rate = 0.005f,
        .thermal_conductivity = 0.3f,
        .flags = FLAG_DECAYING,
        .color_r = 34, .color_g = 139, .color_b = 34
    },
    [MATERIAL_DUST] = {
        .name = "Dust",
        .base_mass = 0.05f,
        .erosion_rate = 0.002f,
        .thermal_conductivity = 0.2f,
        .flags = FLAG_BARELY_DENSE,
        .color_r = 210, .color_g = 180, .color_b = 140
    },
    [MATERIAL_PLASMA] = {
        .name = "Plasma",
        .base_mass = 0.001f,
        .erosion_rate = 0.1f,
        .thermal_conductivity = 10.0f,
        .flags = FLAG_CHARGED | FLAG_FUEL,
        .color_r = 255, .color_g = 100, .color_b = 100
    },
    [MATERIAL_DEBRIS] = {
        .name = "Debris",
        .base_mass = 0.5f,
        .erosion_rate = 0.003f,
        .thermal_conductivity = 0.4f,
        .flags = FLAG_NONE,
        .color_r = 128, .color_g = 128, .color_b = 128
    },
    [MATERIAL_MINERAL] = {
        .name = "Mineral",
        .base_mass = 1.5f,
        .erosion_rate = 0.0005f,
        .thermal_conductivity = 1.0f,
        .flags = FLAG_DENSE,
        .color_r = 255, .color_g = 215, .color_b = 0
    },
    [MATERIAL_ASH] = {
        .name = "Ash",
        .base_mass = 0.1f,
        .erosion_rate = 0.0f,
        .thermal_conductivity = 0.15f,
        .flags = FLAG_BARELY_DENSE,
        .color_r = 105, .color_g = 105, .color_b = 105
    },
    [MATERIAL_CRYSTAL] = {
        .name = "Crystal",
        .base_mass = 1.2f,
        .erosion_rate = 0.0002f,
        .thermal_conductivity = 3.0f,
        .flags = FLAG_REFLECTIVE,
        .color_r = 175, .color_g = 238, .color_b = 238
    },
    [MATERIAL_MAGMA] = {
        .name = "Magma",
        .base_mass = 2.0f,
        .erosion_rate = 0.02f,
        .thermal_conductivity = 2.5f,
        .flags = FLAG_DENSE | FLAG_VOLATILE,
        .color_r = 255, .color_g = 69, .color_b = 0
    },
    [MATERIAL_VAPOR] = {
        .name = "Vapor",
        .base_mass = 0.02f,
        .erosion_rate = 0.05f,
        .thermal_conductivity = 0.1f,
        .flags = FLAG_BUOYANT,
        .color_r = 200, .color_g = 200, .color_b = 220
    },
    [MATERIAL_COMET_ICE] = {
        .name = "Comet Ice",
        .base_mass = 0.7f,
        .erosion_rate = 0.03f,
        .thermal_conductivity = 0.5f,
        .flags = FLAG_VOLATILE | FLAG_BUOYANT,
        .color_r = 135, .color_g = 206, .color_b = 250
    },
    [MATERIAL_DARK_MATTER] = {
        .name = "Dark Matter",
        .base_mass = 0.001f,
        .erosion_rate = 0.0f,
        .thermal_conductivity = 0.0f,
        .flags = FLAG_BARELY_DENSE | FLAG_CHARGED | FLAG_MAGNETIC,
        .color_r = 20, .color_g = 0, .color_b = 40
    }
};