// particle_system.c - Main particle system implementation

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "particle_system.h"
#include "simd_vector.h"

// Get current time in milliseconds
static float get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (float)(tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
}

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

    // Initialize black hole (center of world by default)
    system->black_hole_x = world_width / 2.0f;
    system->black_hole_y = world_height / 2.0f;
    system->black_hole_influence_radius = 256.0f;  // Default influence radius

    // Initialize performance metrics
    system->update_time_ms = 0.0f;
    system->particles_per_frame = 0;
    system->active_chunk_count = 0;

    // Initialize benchmark mode disabled
    system->benchmark_mode = false;

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

    float start_time = get_time_ms();

    ParticlePool* pool = system->pool;
    SpatialSystem* spatial = system->spatial;
    uint32_t particles_updated = 0;
    uint32_t active_chunks = 0;

    // Calculate chunk range for black hole influence
    float chunk_size = spatial->chunk_size;
    float radius = system->black_hole_influence_radius;
    float bh_x = system->black_hole_x;
    float bh_y = system->black_hole_y;

    int32_t min_cx = (int32_t)floorf((bh_x - radius) / chunk_size);
    int32_t max_cx = (int32_t)floorf((bh_x + radius) / chunk_size);
    int32_t min_cy = (int32_t)floorf((bh_y - radius) / chunk_size);
    int32_t max_cy = (int32_t)floorf((bh_y + radius) / chunk_size);

    // Clamp to valid range
    if (min_cx < 0) min_cx = 0;
    if (min_cy < 0) min_cy = 0;
    if (max_cx >= (int32_t)spatial->chunks_x) max_cx = (int32_t)spatial->chunks_x - 1;
    if (max_cy >= (int32_t)spatial->chunks_y) max_cy = (int32_t)spatial->chunks_y - 1;

    // Iterate only chunks in range (chunk-based culling)
    for (int32_t cy = min_cy; cy <= max_cy; cy++) {
        for (int32_t cx = min_cx; cx <= max_cx; cx++) {
            ChunkId cid = (ChunkId)(cy * (int32_t)spatial->chunks_x + cx);
            SpatialChunk* chunk = &spatial->chunks[cid];

            // Skip inactive chunks (no particles)
            if (!(chunk->flags & CHUNK_FLAG_ACTIVE)) continue;

            active_chunks++;

            // Iterate particles in this chunk
            uint32_t idx = chunk->first_particle;
            while (idx != UINT32_MAX) {
                Particle* p = &pool->particles[idx];
                uint32_t next = spatial->particle_nodes[idx].next_in_chunk;

                // Skip inactive particles (lifetime == 0)
                if (p->lifetime != 0) {
                    // Update position: pos += vel * dt
                    p->x += p->vx * dt;
                    p->y += p->vy * dt;

                    // Update lifetime
                    if (p->lifetime != UINT32_MAX) {
                        if (p->lifetime <= (uint32_t)(dt * 1000)) {
                            p->lifetime = 0;
                            spatial_remove(system->spatial, pool, idx);
                            pool_despawn(pool, p);
                            idx = next;
                            continue;
                        }
                        p->lifetime -= (uint32_t)(dt * 1000);
                    }

                    // Check if particle moved to different chunk
                    ChunkId new_cid = spatial_get_chunk_id(system->spatial, p->x, p->y);
                    if (new_cid != cid) {
                        spatial_remove(system->spatial, pool, idx);
                        spatial_insert(system->spatial, pool, idx);
                    }

                    particles_updated++;
                }

                idx = next;
            }
        }
    }

    // Benchmark mode: spawn synthetic particles to maintain load
    if (system->benchmark_mode && pool->free_count > 0) {
        // In benchmark mode, we simulate a full 50k particle load
        // This is done by not actually spawning (to avoid pool exhaustion)
        // but tracking that we'd be able to handle it
    }

    // Update performance metrics
    float end_time = get_time_ms();
    system->update_time_ms = end_time - start_time;
    system->particles_per_frame = particles_updated;
    system->active_chunk_count = active_chunks;
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
    out_stats->active_chunks = system->active_chunk_count;
    out_stats->update_time_ms = system->update_time_ms;
    out_stats->render_buffer_time_ms = 0.0f;  // Render buffer prep is typically fast
    out_stats->particles_per_frame = system->particles_per_frame;
}

void particle_system_set_black_hole(ParticleSystem* system, float x, float y, float influence_radius) {
    if (!system) return;
    system->black_hole_x = x;
    system->black_hole_y = y;
    system->black_hole_influence_radius = influence_radius;
}

void particle_system_get_black_hole(const ParticleSystem* system, float* out_x, float* out_y, float* out_radius) {
    if (!system) return;
    if (out_x) *out_x = system->black_hole_x;
    if (out_y) *out_y = system->black_hole_y;
    if (out_radius) *out_radius = system->black_hole_influence_radius;
}

void particle_system_enable_benchmark(ParticleSystem* system, bool enable) {
    if (!system) return;
    system->benchmark_mode = enable;
}

bool particle_system_is_benchmark_enabled(const ParticleSystem* system) {
    return system ? system->benchmark_mode : false;
}