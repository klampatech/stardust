// black_hole.c - Black hole gravitational mechanics implementation

#include <stdlib.h>
#include <math.h>
#include "black_hole.h"
#include "particle_pool.h"

// Gravitational constant (tuned for gameplay feel)
#define GRAVITATIONAL_CONSTANT 500.0f

// Tidal force exponent (controls spaghettification steepness)
#define TIDAL_EXPONENT 2.0f

// Minimum distance to avoid singularity
#define MIN_DISTANCE 0.1f

BlackHole* black_hole_create(float x, float y, const BlackHoleConfig* config) {
    if (!config) return NULL;

    BlackHole* bh = (BlackHole*)malloc(sizeof(BlackHole));
    if (!bh) return NULL;

    bh->x = x;
    bh->y = y;
    bh->mass = config->mass;
    bh->destruction_radius = config->destruction_radius;
    bh->influence_radius = config->influence_radius;
    bh->destruction_radius_sq = config->destruction_radius * config->destruction_radius;
    bh->influence_radius_sq = config->influence_radius * config->influence_radius;
    bh->particles_consumed = 0;
    bh->total_mass_consumed = 0.0f;

    return bh;
}

void black_hole_destroy(BlackHole* bh) {
    if (bh) free(bh);
}

void black_hole_configure(BlackHole* bh, const BlackHoleConfig* config) {
    if (!bh || !config) return;

    bh->mass = config->mass;
    bh->destruction_radius = config->destruction_radius;
    bh->influence_radius = config->influence_radius;
    bh->destruction_radius_sq = config->destruction_radius * config->destruction_radius;
    bh->influence_radius_sq = config->influence_radius * config->influence_radius;
}

BlackHoleConfig black_hole_get_config(const BlackHole* bh) {
    BlackHoleConfig config = {0};
    if (!bh) return config;

    config.mass = bh->mass;
    config.destruction_radius = bh->destruction_radius;
    config.influence_radius = bh->influence_radius;
    return config;
}

void black_hole_set_position(BlackHole* bh, float x, float y) {
    if (bh) {
        bh->x = x;
        bh->y = y;
    }
}

void black_hole_get_position(const BlackHole* bh, float* out_x, float* out_y) {
    if (bh) {
        if (out_x) *out_x = bh->x;
        if (out_y) *out_y = bh->y;
    }
}

void black_hole_get_stats(const BlackHole* bh, BlackHoleStats* out_stats) {
    if (!bh || !out_stats) return;

    out_stats->particles_consumed = bh->particles_consumed;
    out_stats->total_mass_consumed = bh->total_mass_consumed;
    // Gravitational strength at the edge of influence radius: F = G * M / r^2
    out_stats->gravitational_strength = GRAVITATIONAL_CONSTANT * bh->mass / bh->influence_radius_sq;
}

void black_hole_reset_stats(BlackHole* bh) {
    if (bh) {
        bh->particles_consumed = 0;
        bh->total_mass_consumed = 0.0f;
    }
}

bool black_hole_is_within_destruction_radius(const BlackHole* bh, float x, float y) {
    if (!bh) return false;

    float dx = x - bh->x;
    float dy = y - bh->y;
    float dist_sq = dx * dx + dy * dy;

    return dist_sq <= bh->destruction_radius_sq;
}

bool black_hole_is_within_influence_radius(const BlackHole* bh, float x, float y) {
    if (!bh) return false;

    float dx = x - bh->x;
    float dy = y - bh->y;
    float dist_sq = dx * dx + dy * dy;

    return dist_sq <= bh->influence_radius_sq;
}

float black_hole_calculate_tidal_factor(const BlackHole* bh, float particle_x, float particle_y) {
    if (!bh) return 1.0f;

    float dx = particle_x - bh->x;
    float dy = particle_y - bh->y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < bh->destruction_radius) {
        // Inside event horizon - maximum tidal stretching
        return 5.0f;
    }

    if (dist > bh->influence_radius) {
        // Outside influence - no tidal effect
        return 1.0f;
    }

    // Calculate tidal factor based on distance from event horizon
    // Normalized distance from destruction radius to influence edge
    float normalized_dist = (dist - bh->destruction_radius) / (bh->influence_radius - bh->destruction_radius);

    // Tidal factor increases as we approach the event horizon
    // Using inverse power to create steep increase near destruction radius
    float tidal = 1.0f + (1.0f - normalized_dist) * (1.0f - normalized_dist) * (TIDAL_EXPONENT * 2.0f);

    return tidal;
}

uint32_t black_hole_apply_gravity(BlackHole* bh, ParticlePool* pool, float dt) {
    if (!bh || !pool) return 0;

    uint32_t destroyed = 0;

    for (uint32_t i = 0; i < pool->max_particles; i++) {
        Particle* p = &pool->particles[i];

        // Skip inactive particles
        if (p->lifetime == 0) continue;

        float dx = bh->x - p->x;
        float dy = bh->y - p->y;
        float dist_sq = dx * dx + dy * dy;

        // Check destruction radius first
        if (dist_sq <= bh->destruction_radius_sq) {
            // Particle is inside event horizon - destroy it
            bh->particles_consumed++;
            bh->total_mass_consumed += p->mass;
            p->lifetime = 0;
            pool_despawn(pool, p);
            destroyed++;
            continue;
        }

        // Check influence radius
        if (dist_sq > bh->influence_radius_sq) {
            continue;
        }

        // Calculate distance
        float dist = sqrtf(dist_sq);
        if (dist < MIN_DISTANCE) continue;

        // Calculate gravitational force: F = G * M / r^2
        float force_magnitude = GRAVITATIONAL_CONSTANT * bh->mass / dist_sq;

        // Normalize direction vector
        float nx = dx / dist;
        float ny = dy / dist;

        // Apply gravitational acceleration: a = F / m (but we use mass scaling for gameplay)
        float ax = nx * force_magnitude * dt;
        float ay = ny * force_magnitude * dt;

        // Calculate tidal factor for stretch effect
        float tidal_factor = black_hole_calculate_tidal_factor(bh, p->x, p->y);

        // Apply tidal scaling to acceleration (stretch effect near black hole)
        p->vx += ax * tidal_factor;
        p->vy += ay * tidal_factor;
    }

    return destroyed;
}
