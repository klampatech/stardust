// black_hole.h - Black hole gravitational mechanics for particle system
// Provides gravitational attraction, event horizon destruction, and tidal effects

#ifndef BLACK_HOLE_H
#define BLACK_HOLE_H

#include <stdint.h>
#include <stdbool.h>
#include "particle_pool.h"

// Black hole configuration
typedef struct BlackHoleConfig {
    float mass;              // Mass of black hole (gravitational strength)
    float destruction_radius; // Event horizon radius (particles inside are destroyed)
    float influence_radius;   // Radius of gravitational influence
} BlackHoleConfig;

// Black hole statistics
typedef struct BlackHoleStats {
    uint32_t particles_consumed;  // Cumulative particles destroyed
    float total_mass_consumed;    // Cumulative mass consumed
    float gravitational_strength; // Current gravitational pull at influence edge
} BlackHoleStats;

// Main black hole instance
typedef struct BlackHole {
    float x, y;                    // Position
    float mass;                    // Gravitational mass
    float destruction_radius;      // Event horizon radius
    float influence_radius;         // Gravitational influence radius
    float destruction_radius_sq;   // Precomputed squared destruction radius
    float influence_radius_sq;     // Precomputed squared influence radius
    uint32_t particles_consumed;   // Total particles consumed
    float total_mass_consumed;     // Total mass consumed
} BlackHole;

// Black hole lifecycle
BlackHole* black_hole_create(float x, float y, const BlackHoleConfig* config);
void black_hole_destroy(BlackHole* bh);

// Configuration
void black_hole_configure(BlackHole* bh, const BlackHoleConfig* config);
BlackHoleConfig black_hole_get_config(const BlackHole* bh);

// Position
void black_hole_set_position(BlackHole* bh, float x, float y);
void black_hole_get_position(const BlackHole* bh, float* out_x, float* out_y);

// Statistics
void black_hole_get_stats(const BlackHole* bh, BlackHoleStats* out_stats);
void black_hole_reset_stats(BlackHole* bh);

// Physics: Apply gravitational force to particles within influence radius
// Returns number of particles destroyed
uint32_t black_hole_apply_gravity(BlackHole* bh, ParticlePool* pool, float dt);

// Physics: Calculate tidal force factor for a particle at given distance
// Returns tidal factor (1.0 = normal, >1.0 = stretched)
// Exposed via particle buffer for rendering to read
float black_hole_calculate_tidal_factor(const BlackHole* bh, float particle_x, float particle_y);

// Query: Check if a point is within the destruction radius
bool black_hole_is_within_destruction_radius(const BlackHole* bh, float x, float y);

// Query: Check if a point is within the influence radius
bool black_hole_is_within_influence_radius(const BlackHole* bh, float x, float y);

#endif // BLACK_HOLE_H
