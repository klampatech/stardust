// particle_pool.h - Pre-allocated particle pool allocator
// No malloc/free during runtime - all memory allocated upfront

#ifndef PARTICLE_POOL_H
#define PARTICLE_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "material_types.h"

// Forward declarations
typedef struct Particle Particle;
typedef struct ParticlePool ParticlePool;

// Maximum particles supported
#define MAX_PARTICLES 65536  // 64k max, allows headroom beyond 50k requirement

// Particle data structure (packed for cache efficiency)
struct Particle {
    float x, y;           // Position (vec2)
    float vx, vy;         // Velocity (vec2)
    uint8_t material;     // Material type (0-14)
    uint8_t flags;        // Per-particle state flags
    uint16_t chunk_id;    // Current spatial chunk
    float mass;            // Current mass (can change with erosion)
    uint32_t lifetime;    // Remaining lifetime in ticks
    uint32_t id;          // Unique particle identifier
};

// Pool allocator structure
struct ParticlePool {
    Particle* particles;      // Flat array of particles
    uint32_t* free_list;      // Stack of available slot indices
    uint32_t free_count;      // Number of free slots
    uint32_t active_count;    // Number of currently active particles
    uint32_t max_particles;   // Pool capacity
    uint32_t next_id;         // ID counter for unique particle IDs
};

// Pool lifecycle
ParticlePool* pool_create(uint32_t max_particles);
void pool_destroy(ParticlePool* pool);

// Particle allocation/deallocation (O(1))
Particle* pool_spawn(ParticlePool* pool);
void pool_despawn(ParticlePool* pool, Particle* p);

// Bulk operations
Particle* pool_spawnn(ParticlePool* pool, uint32_t count);
void pool_despawn_all(ParticlePool* pool);

// Query functions
static inline uint32_t pool_active_count(const ParticlePool* pool) {
    return pool->active_count;
}

static inline uint32_t pool_free_count(const ParticlePool* pool) {
    return pool->free_count;
}

static inline bool pool_is_full(const ParticlePool* pool) {
    return pool->free_count == 0;
}

static inline bool pool_is_empty(const ParticlePool* pool) {
    return pool->active_count == 0;
}

static inline Particle* pool_get(const ParticlePool* pool, uint32_t index) {
    return (index < pool->max_particles) ? &pool->particles[index] : NULL;
}

#endif // PARTICLE_POOL_H