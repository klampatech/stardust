// material_types.h - Material type definitions for particle system
// Each material has unique behavior flags for physics simulation

#ifndef MATERIAL_TYPES_H
#define MATERIAL_TYPES_H

#include <stdint.h>

// Material type IDs (0-14, allowing 15 distinct materials)
typedef enum {
    MATERIAL_ROCK = 0,
    MATERIAL_ICE = 1,
    MATERIAL_METAL = 2,
    MATERIAL_GAS = 3,
    MATERIAL_ORGANIC = 4,
    MATERIAL_DUST = 5,
    MATERIAL_PLASMA = 6,
    MATERIAL_DEBRIS = 7,
    MATERIAL_MINERAL = 8,
    MATERIAL_ASH = 9,
    MATERIAL_CRYSTAL = 10,
    MATERIAL_MAGMA = 11,
    MATERIAL_VAPOR = 12,
    MATERIAL_COMET_ICE = 13,
    MATERIAL_DARK_MATTER = 14,
    MATERIAL_COUNT = 15
} MaterialType;

// Material behavior flags (bitmask)
typedef enum {
    FLAG_NONE         = 0,
    FLAG_FUEL         = (1 << 0),   // Can ignite/burn
    FLAG_TOXIC        = (1 << 1),   // Hazardous to nearby particles
    FLAG_MAGNETIC     = (1 << 2),   // Affected by magnetic fields
    FLAG_CHARGED      = (1 << 3),   // Carries electric charge
    FLAG_VOLATILE     = (1 << 4),   // Explodes under pressure
    FLAG_ABSORBENT    = (1 << 5),   // Absorbs other particles
    FLAG_REFLECTIVE   = (1 << 6),   // Reflects energy
    FLAG_DENSE        = (1 << 7),   // High mass per volume
    FLAG_BUOYANT      = (1 << 8),   // Rises in gravity wells
    FLAG_DECAYING     = (1 << 9),   // Breaks down over time
    FLAG_BARELY_DENSE = (1 << 10),  // Low mass per volume
} MaterialFlags;

// Material properties table
typedef struct {
    const char* name;
    float base_mass;          // Base mass in kg
    float erosion_rate;       // How fast material erodes
    float thermal_conductivity; // Heat transfer rate
    uint16_t flags;           // Behavior flags
    uint8_t color_r;          // Visual color components
    uint8_t color_g;
    uint8_t color_b;
} MaterialProperties;

// Global material database
extern const MaterialProperties MATERIAL_TABLE[MATERIAL_COUNT];

// Helper functions
static inline uint16_t material_get_flags(MaterialType type) {
    return MATERIAL_TABLE[type].flags;
}

static inline float material_get_mass(MaterialType type) {
    return MATERIAL_TABLE[type].base_mass;
}

static inline uint32_t material_pack_color(MaterialType type) {
    const MaterialProperties* m = &MATERIAL_TABLE[type];
    return ((uint32_t)m->color_r << 16) | ((uint32_t)m->color_g << 8) | m->color_b;
}

#endif // MATERIAL_TYPES_H