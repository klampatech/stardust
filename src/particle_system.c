// particle_system.c - Main particle system implementation

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "particle_system.h"

static float rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

static float rand_range(float min, float max) {
    return min + rand_float() * (max - min);
}

ParticleSystem* particle_system_create(float world_width, float world_height, uint32_t max_particles) {
    if (max_particles > MAX_PARTICLES) max_particles = MAX_PARTICLES;

    ParticleSystem* system = (ParticleSystem*)malloc(sizeof(ParticleSystem));
    if (!system) return NULL;

    system->pool = pool_create(max_particles);
    if (!system->pool) {
        free(system);
        return NULL;
    }

    system->spatial = spatial_create(world_width, world_height, max_particles, 0.0f);  // 0 = use default chunk size
    if (!system->spatial) {
        pool_destroy(system->pool);
        free(system);
        return NULL;
    }

    system->world_width = world_width;
    system->world_height = world_height;
    system->template_count = 0;
    system->black_hole = NULL;  // No black hole initially
    system->initialized = true;

    // Initialize render buffer
    system->render_buffer.positions = (float*)malloc(sizeof(float) * 2 * max_particles);
    system->render_buffer.velocities = (float*)malloc(sizeof(float) * 2 * max_particles);
    system->render_buffer.materials = (uint8_t*)malloc(sizeof(uint8_t) * max_particles);
    system->render_buffer.masses = (float*)malloc(sizeof(float) * max_particles);
    system->render_buffer.lifetimes = (uint32_t*)malloc(sizeof(uint32_t) * max_particles);
    system->render_buffer.ids = (uint32_t*)malloc(sizeof(uint32_t) * max_particles);
    system->render_buffer.capacity = max_particles;
    system->render_buffer.count = 0;

    // Seed random
    srand((uint32_t)time(NULL));

    return system;
}

void particle_system_destroy(ParticleSystem* system) {
    if (!system) return;

    if (system->render_buffer.positions) free(system->render_buffer.positions);
    if (system->render_buffer.velocities) free(system->render_buffer.velocities);
    if (system->render_buffer.materials) free(system->render_buffer.materials);
    if (system->render_buffer.masses) free(system->render_buffer.masses);
    if (system->render_buffer.lifetimes) free(system->render_buffer.lifetimes);
    if (system->render_buffer.ids) free(system->render_buffer.ids);

    if (system->black_hole) black_hole_destroy(system->black_hole);
    if (system->spatial) spatial_destroy(system->spatial);
    if (system->pool) pool_destroy(system->pool);

    free(system);
}

int particle_system_register_template(ParticleSystem* system, const SpawnTemplate* tmpl) {
    if (!system || !tmpl || system->template_count >= 16) return -1;

    int id = system->template_count++;
    system->templates[id] = *tmpl;
    return id;
}

const SpawnTemplate* particle_system_get_template(const ParticleSystem* system, int template_id) {
    if (!system || template_id < 0 || template_id >= system->template_count) return NULL;
    return &system->templates[template_id];
}

void particle_system_set_black_hole(ParticleSystem* system, BlackHole* bh) {
    if (!system) return;
    if (system->black_hole) {
        black_hole_destroy(system->black_hole);
    }
    system->black_hole = bh;
}

BlackHole* particle_system_get_black_hole(const ParticleSystem* system) {
    return system ? system->black_hole : NULL;
}

const BlackHoleStats* particle_system_get_black_hole_stats(const ParticleSystem* system) {
    static BlackHoleStats stats_zero = {0};
    if (!system || !system->black_hole) return &stats_zero;

    static BlackHoleStats cached_stats;
    black_hole_get_stats(system->black_hole, &cached_stats);
    return &cached_stats;
}

uint32_t particle_system_spawn_cluster(ParticleSystem* system, float pos_x, float pos_y,
                                       int template_id, uint32_t count) {
    if (!system || template_id < 0 || template_id >= system->template_count) return 0;
    if (count == 0 || count > system->pool->free_count) count = system->pool->free_count;
    if (count == 0) return 0;

    const SpawnTemplate* tmpl = &system->templates[template_id];
    uint32_t spawned = 0;

    for (uint32_t i = 0; i < count; i++) {
        Particle* p = pool_spawn(system->pool);
        if (!p) break;

        // Random position within spread radius
        float angle = rand_float() * 2.0f * 3.14159265f;
        float r = sqrtf(rand_float()) * tmpl->spread_radius;
        p->x = pos_x + cosf(angle) * r;
        p->y = pos_y + sinf(angle) * r;

        // Random velocity direction, magnitude within range
        float v_angle = rand_float() * 2.0f * 3.14159265f;
        float v_mag = rand_range(tmpl->velocity_min, tmpl->velocity_max);
        p->vx = cosf(v_angle) * v_mag;
        p->vy = sinf(v_angle) * v_mag;

        // Material and mass
        p->material = tmpl->material;
        p->mass = MATERIAL_TABLE[tmpl->material].base_mass * tmpl->mass_modifier;

        // Lifetime
        p->lifetime = tmpl->base_lifetime;

        // Add to spatial system
        spatial_insert(system->spatial, system->pool, (uint32_t)(p - system->pool->particles));

        spawned++;
    }

    return spawned;
}

void particle_system_despawn_particle(ParticleSystem* system, uint32_t particle_id) {
    if (!system) return;

    ParticlePool* pool = system->pool;
    for (uint32_t i = 0; i < pool->max_particles; i++) {
        if (pool->particles[i].id == particle_id && pool->particles[i].lifetime > 0) {
            spatial_remove(system->spatial, pool, i);
            pool_despawn(pool, &pool->particles[i]);
            return;
        }
    }
}

void particle_system_despawn_all(ParticleSystem* system) {
    if (!system) return;
    pool_despawn_all(system->pool);
}

void particle_system_update(ParticleSystem* system, float dt) {
    if (!system || !system->initialized) return;

    // Apply black hole gravity first (may destroy particles)
    if (system->black_hole) {
        black_hole_apply_gravity(system->black_hole, system->pool, dt);
    }

    ParticlePool* pool = system->pool;

    for (uint32_t i = 0; i < pool->max_particles; i++) {
        Particle* p = &pool->particles[i];

        // Skip inactive particles
        if (p->lifetime == 0) continue;

        // Update position
        float new_x = p->x + p->vx * dt;
        float new_y = p->y + p->vy * dt;

        // Update lifetime
        if (p->lifetime != UINT32_MAX) {
            if (p->lifetime <= (uint32_t)(dt * 1000)) {
                p->lifetime = 0;
                spatial_remove(system->spatial, pool, i);
                pool_despawn(pool, p);
                continue;
            }
            p->lifetime -= (uint32_t)(dt * 1000);
        }

        // Update spatial if moved to different chunk
        ChunkId old_cid = p->chunk_id;
        ChunkId new_cid = spatial_get_chunk_id(system->spatial, new_x, new_y);
        if (old_cid != new_cid) {
            spatial_remove(system->spatial, pool, i);
            p->x = new_x;
            p->y = new_y;
            spatial_insert(system->spatial, pool, i);
        } else {
            p->x = new_x;
            p->y = new_y;
        }
    }
}

void particle_system_prepare_render_buffer(ParticleSystem* system) {
    if (!system) return;

    ParticlePool* pool = system->pool;
    ParticleBuffer* buf = &system->render_buffer;

    uint32_t idx = 0;
    for (uint32_t i = 0; i < pool->max_particles && idx < buf->capacity; i++) {
        Particle* p = &pool->particles[i];
        if (p->lifetime == 0) continue;

        buf->positions[idx * 2] = p->x;
        buf->positions[idx * 2 + 1] = p->y;
        buf->velocities[idx * 2] = p->vx;
        buf->velocities[idx * 2 + 1] = p->vy;
        buf->materials[idx] = p->material;
        buf->masses[idx] = p->mass;
        buf->lifetimes[idx] = p->lifetime;
        buf->ids[idx] = p->id;
        idx++;
    }
    buf->count = idx;
}

const ParticleBuffer* particle_system_get_buffer(const ParticleSystem* system) {
    return system ? &system->render_buffer : NULL;
}

uint32_t particle_system_get_count(const ParticleSystem* system) {
    return system ? system->pool->active_count : 0;
}

bool particle_system_get_particle(const ParticleSystem* system, uint32_t id,
                                   float* out_x, float* out_y,
                                   float* out_vx, float* out_vy,
                                   uint8_t* out_material,
                                   float* out_mass, uint32_t* out_lifetime) {
    if (!system) return false;

    ParticlePool* pool = system->pool;
    for (uint32_t i = 0; i < pool->max_particles; i++) {
        Particle* p = &pool->particles[i];
        if (p->id == id && p->lifetime > 0) {
            if (out_x) *out_x = p->x;
            if (out_y) *out_y = p->y;
            if (out_vx) *out_vx = p->vx;
            if (out_vy) *out_vy = p->vy;
            if (out_material) *out_material = p->material;
            if (out_mass) *out_mass = p->mass;
            if (out_lifetime) *out_lifetime = p->lifetime;
            return true;
        }
    }
    return false;
}

void particle_system_get_stats(const ParticleSystem* system, ParticleSystemStats* out_stats) {
    if (!system || !out_stats) return;

    out_stats->active_particles = system->pool->active_count;
    out_stats->free_slots = system->pool->free_count;
    out_stats->active_chunks = system->spatial->active_chunk_count;
    out_stats->update_time_ms = 0.0f;
    out_stats->render_buffer_time_ms = 0.0f;
}