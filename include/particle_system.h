// particle_system.h - Main particle system interface
// C engine for Godot space game with black hole particle simulation

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "particle_pool.h"
#include "spatial_chunks.h"
#include "material_types.h"
#include "black_hole.h"

// Spawn template for cluster creation
typedef struct SpawnTemplate {
    uint8_t material;         // Material type for spawned particles
    float spread_radius;     // Initial position spread
    float velocity_min;       // Minimum initial speed
    float velocity_max;       // Maximum initial speed
    float mass_modifier;      // Mass multiplier
    uint32_t base_lifetime;   // Base lifetime in ticks
} SpawnTemplate;

// Read-only particle buffer for Godot (no malloc on Godot side)
typedef struct ParticleBuffer {
    float* positions;        // Interleaved x,y pairs [x0,y0,x1,y1,...]
    float* velocities;        // Interleaved vx,vy pairs
    uint8_t* materials;       // Material type array
    float* masses;            // Mass array
    uint32_t* lifetimes;      // Lifetime array
    uint32_t* ids;            // Unique ID array
    uint32_t count;           // Active particle count
    uint32_t capacity;        // Buffer capacity
} ParticleBuffer;

// Main particle system instance
typedef struct ParticleSystem {
    ParticlePool* pool;
    SpatialSystem* spatial;
    BlackHole* black_hole;           // Central black hole (may be NULL)
    SpawnTemplate templates[16];  // Up to 16 spawn templates
    uint8_t template_count;
    ParticleBuffer render_buffer;  // Read-only buffer for Godot
    float world_width;
    float world_height;
    bool initialized;
} ParticleSystem;

// System lifecycle
ParticleSystem* particle_system_create(float world_width, float world_height, uint32_t max_particles);
void particle_system_destroy(ParticleSystem* system);

// Template management
int particle_system_register_template(ParticleSystem* system, const SpawnTemplate* tmpl);
const SpawnTemplate* particle_system_get_template(const ParticleSystem* system, int template_id);

// Black hole management
void particle_system_set_black_hole(ParticleSystem* system, BlackHole* bh);
BlackHole* particle_system_get_black_hole(const ParticleSystem* system);
const BlackHoleStats* particle_system_get_black_hole_stats(const ParticleSystem* system);

// Particle spawning
uint32_t particle_system_spawn_cluster(ParticleSystem* system, float pos_x, float pos_y,
                                      int template_id, uint32_t count);
void particle_system_despawn_particle(ParticleSystem* system, uint32_t particle_id);
void particle_system_despawn_all(ParticleSystem* system);

// Per-frame update (call from physics thread)
void particle_system_update(ParticleSystem* system, float dt);

// Render buffer update (call from main thread before Godot render)
void particle_system_prepare_render_buffer(ParticleSystem* system);
const ParticleBuffer* particle_system_get_buffer(const ParticleSystem* system);

// Particle queries
uint32_t particle_system_get_count(const ParticleSystem* system);
bool particle_system_get_particle(const ParticleSystem* system, uint32_t id,
                                   float* out_x, float* out_y,
                                   float* out_vx, float* out_vy,
                                   uint8_t* out_material,
                                   float* out_mass, uint32_t* out_lifetime);

// Statistics
typedef struct ParticleSystemStats {
    uint32_t active_particles;
    uint32_t free_slots;
    uint32_t active_chunks;
    float update_time_ms;
    float render_buffer_time_ms;
} ParticleSystemStats;

void particle_system_get_stats(const ParticleSystem* system, ParticleSystemStats* out_stats);

#endif // PARTICLE_SYSTEM_H