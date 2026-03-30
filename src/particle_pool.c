// particle_pool.c - Pre-allocated particle pool implementation

#include <stdlib.h>
#include <string.h>
#include "particle_pool.h"

ParticlePool* pool_create(uint32_t max_particles) {
    ParticlePool* pool = (ParticlePool*)malloc(sizeof(ParticlePool));
    if (!pool) return NULL;

    pool->particles = (Particle*)malloc(sizeof(Particle) * max_particles);
    if (!pool->particles) {
        free(pool);
        return NULL;
    }

    pool->free_list = (uint32_t*)malloc(sizeof(uint32_t) * max_particles);
    if (!pool->free_list) {
        free(pool->particles);
        free(pool);
        return NULL;
    }

    // Initialize free list as stack (all slots available)
    for (uint32_t i = 0; i < max_particles; i++) {
        pool->free_list[i] = i;
    }

    pool->free_count = max_particles;
    pool->active_count = 0;
    pool->max_particles = max_particles;
    pool->next_id = 0;

    // Zero all particle memory for safety
    memset(pool->particles, 0, sizeof(Particle) * max_particles);

    return pool;
}

void pool_destroy(ParticlePool* pool) {
    if (!pool) return;
    free(pool->particles);
    free(pool->free_list);
    free(pool);
}

Particle* pool_spawn(ParticlePool* pool) {
    if (pool->free_count == 0) return NULL;

    // Pop from free list stack
    uint32_t idx = pool->free_list[--pool->free_count];
    Particle* p = &pool->particles[idx];

    // Initialize particle
    p->x = p->y = 0.0f;
    p->vx = p->vy = 0.0f;
    p->material = 0;
    p->flags = 0;
    p->chunk_id = UINT16_MAX;
    p->mass = 0.0f;
    p->lifetime = 0;
    p->id = pool->next_id++;

    pool->active_count++;
    return p;
}

void pool_despawn(ParticlePool* pool, Particle* p) {
    if (!p) return;

    uint32_t idx = (uint32_t)(p - pool->particles);
    if (idx >= pool->max_particles) return;  // Invalid pointer

    // Mark as inactive
    p->flags = 0;
    p->chunk_id = UINT16_MAX;

    // Push to free list stack
    pool->free_list[pool->free_count++] = idx;
    pool->active_count--;
}

Particle* pool_spawnn(ParticlePool* pool, uint32_t count) {
    if (count == 0) return NULL;
    if (count > pool->free_count) count = pool->free_count;

    Particle* first = &pool->particles[pool->free_list[pool->free_count - 1]];

    // Allocate batch
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = pool->free_list[--pool->free_count];
        Particle* p = &pool->particles[idx];

        // Initialize particle
        p->x = p->y = 0.0f;
        p->vx = p->vy = 0.0f;
        p->material = 0;
        p->flags = 0;
        p->chunk_id = UINT16_MAX;
        p->mass = 0.0f;
        p->lifetime = 0;
        p->id = pool->next_id++;

        // Push previous to free list for individual despawn
        if (i > 0) {
            // Maintain free list - particles allocated in reverse order
        }
    }

    pool->active_count += count;
    return first;
}

void pool_despawn_all(ParticlePool* pool) {
    // Reset free list
    for (uint32_t i = 0; i < pool->max_particles; i++) {
        pool->free_list[i] = i;
    }
    pool->free_count = pool->max_particles;
    pool->active_count = 0;

    // Mark all particles inactive
    for (uint32_t i = 0; i < pool->max_particles; i++) {
        pool->particles[i].flags = 0;
        pool->particles[i].chunk_id = UINT16_MAX;
    }
}