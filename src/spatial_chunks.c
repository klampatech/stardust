// spatial_chunks.c - Chunk-based spatial organization implementation

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "spatial_chunks.h"

SpatialSystem* spatial_create(float world_width, float world_height, uint32_t max_particles) {
    SpatialSystem* system = (SpatialSystem*)malloc(sizeof(SpatialSystem));
    if (!system) return NULL;

    system->particle_nodes = (ChunkParticleNode*)malloc(sizeof(ChunkParticleNode) * max_particles);
    if (!system->particle_nodes) {
        free(system);
        return NULL;
    }

    system->chunk_size = CHUNK_SIZE;
    system->world_width = world_width;
    system->world_height = world_height;
    system->chunks_x = (uint32_t)ceilf(world_width / CHUNK_SIZE);
    system->chunks_y = (uint32_t)ceilf(world_height / CHUNK_SIZE);

    if (system->chunks_x > MAX_CHUNKS_X) system->chunks_x = MAX_CHUNKS_X;
    if (system->chunks_y > MAX_CHUNKS_Y) system->chunks_y = MAX_CHUNKS_Y;
    system->max_chunks = system->chunks_x * system->chunks_y;
    system->active_chunk_count = 0;

    // Initialize all chunks
    for (uint32_t i = 0; i < MAX_CHUNKS; i++) {
        system->chunks[i].first_particle = UINT32_MAX;
        system->chunks[i].particle_count = 0;
        system->chunks[i].x = 0;
        system->chunks[i].y = 0;
        system->chunks[i].flags = 0;
    }

    // Initialize particle nodes
    for (uint32_t i = 0; i < max_particles; i++) {
        system->particle_nodes[i].particle_index = UINT32_MAX;
        system->particle_nodes[i].next_in_chunk = UINT32_MAX;
    }

    return system;
}

void spatial_destroy(SpatialSystem* system) {
    if (!system) return;
    free(system->particle_nodes);
    free(system);
}

ChunkId spatial_get_chunk_id(const SpatialSystem* system, float x, float y) {
    int32_t cx = (int32_t)floorf(x * CHUNK_SIZE_INV);
    int32_t cy = (int32_t)floorf(y * CHUNK_SIZE_INV);

    // Clamp to valid range
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx >= (int32_t)system->chunks_x) cx = (int32_t)system->chunks_x - 1;
    if (cy >= (int32_t)system->chunks_y) cy = (int32_t)system->chunks_y - 1;

    return (ChunkId)(cy * system->chunks_x + cx);
}

void spatial_insert(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx) {
    if (!system || !pool || particle_idx >= pool->max_particles) return;

    Particle* p = &pool->particles[particle_idx];
    ChunkId cid = spatial_get_chunk_id(system, p->x, p->y);
    SpatialChunk* chunk = &system->chunks[cid];

    // Link particle to chunk
    system->particle_nodes[particle_idx].particle_index = particle_idx;
    system->particle_nodes[particle_idx].next_in_chunk = chunk->first_particle;
    chunk->first_particle = particle_idx;
    chunk->particle_count++;
    chunk->flags |= CHUNK_FLAG_ACTIVE | CHUNK_FLAG_DIRTY;

    // Update particle's chunk
    p->chunk_id = (uint16_t)cid;
}

void spatial_remove(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx) {
    if (!system || !pool || particle_idx >= pool->max_particles) return;

    Particle* p = &pool->particles[particle_idx];
    ChunkId cid = p->chunk_id;

    if (cid == UINT16_MAX) return;  // Not in any chunk

    SpatialChunk* chunk = &system->chunks[cid];
    uint32_t prev = UINT32_MAX;
    uint32_t current = chunk->first_particle;

    // Find and unlink particle from chunk's linked list
    while (current != UINT32_MAX) {
        if (current == particle_idx) {
            if (prev == UINT32_MAX) {
                chunk->first_particle = system->particle_nodes[current].next_in_chunk;
            } else {
                system->particle_nodes[prev].next_in_chunk = system->particle_nodes[current].next_in_chunk;
            }
            chunk->particle_count--;
            if (chunk->particle_count == 0) {
                chunk->flags &= ~CHUNK_FLAG_ACTIVE;
            }
            break;
        }
        prev = current;
        current = system->particle_nodes[current].next_in_chunk;
    }

    // Clear particle's chunk
    system->particle_nodes[particle_idx].particle_index = UINT32_MAX;
    system->particle_nodes[particle_idx].next_in_chunk = UINT32_MAX;
    p->chunk_id = UINT16_MAX;
}

void spatial_move(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx, float new_x, float new_y) {
    if (!system || !pool || particle_idx >= pool->max_particles) return;

    Particle* p = &pool->particles[particle_idx];
    ChunkId old_cid = p->chunk_id;
    ChunkId new_cid = spatial_get_chunk_id(system, new_x, new_y);

    // Only update if chunk changed
    if (old_cid != new_cid) {
        spatial_remove(system, pool, particle_idx);
        p->x = new_x;
        p->y = new_y;
        spatial_insert(system, pool, particle_idx);
    } else {
        p->x = new_x;
        p->y = new_y;
        // Mark chunk dirty
        if (new_cid < MAX_CHUNKS) {
            system->chunks[new_cid].flags |= CHUNK_FLAG_DIRTY;
        }
    }
}

SpatialChunk* spatial_get_chunk(SpatialSystem* system, ChunkId id) {
    if (!system || id >= MAX_CHUNKS) return NULL;
    return &system->chunks[id];
}

SpatialChunk* spatial_get_chunk_at(SpatialSystem* system, float x, float y) {
    ChunkId id = spatial_get_chunk_id(system, x, y);
    return spatial_get_chunk(system, id);
}

uint32_t* spatial_get_chunk_particles(SpatialSystem* system, ChunkId id, uint32_t* out_count) {
    static uint32_t particle_buffer[256];  // Max 256 per chunk
    if (!system || id >= MAX_CHUNKS || !out_count) return NULL;

    SpatialChunk* chunk = &system->chunks[id];
    *out_count = (chunk->particle_count < 256) ? chunk->particle_count : 256;

    uint32_t idx = 0;
    uint32_t current = chunk->first_particle;
    while (current != UINT32_MAX && idx < 256) {
        particle_buffer[idx++] = current;
        current = system->particle_nodes[current].next_in_chunk;
    }
    *out_count = idx;

    return particle_buffer;
}

void spatial_iter_chunk_range(SpatialSystem* system, float center_x, float center_y,
                              float radius, ChunkIterator* iter_out) {
    if (!system || !iter_out) return;

    iter_out->system = system;

    // Calculate chunk range
    int32_t min_cx = (int32_t)floorf((center_x - radius) * CHUNK_SIZE_INV);
    int32_t max_cx = (int32_t)floorf((center_x + radius) * CHUNK_SIZE_INV);
    int32_t min_cy = (int32_t)floorf((center_y - radius) * CHUNK_SIZE_INV);
    int32_t max_cy = (int32_t)floorf((center_y + radius) * CHUNK_SIZE_INV);

    // Clamp
    if (min_cx < 0) min_cx = 0;
    if (min_cy < 0) min_cy = 0;
    if (max_cx >= (int32_t)system->chunks_x) max_cx = (int32_t)system->chunks_x - 1;
    if (max_cy >= (int32_t)system->chunks_y) max_cy = (int32_t)system->chunks_y - 1;

    iter_out->cx = min_cx - 1;
    iter_out->cy = min_cy;
    iter_out->dx = max_cx - min_cx + 1;
    iter_out->dy = max_cy - min_cy + 1;
    iter_out->current = UINT32_MAX;
    iter_out->count = 0;
}

bool spatial_iter_next(ChunkIterator* iter, uint32_t* particle_idx_out) {
    if (!iter || !particle_idx_out) return false;

    SpatialSystem* system = iter->system;

    while (iter->cy < (int32_t)(iter->dy + iter->cy - (int32_t)system->chunks_y)) {
        iter->cx++;
        if (iter->cx >= (int32_t)system->chunks_x) {
            iter->cx = 0;
            iter->cy++;
        }
        if (iter->cy >= (int32_t)system->chunks_y) break;

        ChunkId cid = (uint32_t)(iter->cy * MAX_CHUNKS_X + iter->cx);
        SpatialChunk* chunk = &system->chunks[cid];

        if (chunk->particle_count > 0) {
            iter->current = chunk->first_particle;
            iter->count = chunk->particle_count;
            *particle_idx_out = iter->current;
            return true;
        }
    }

    return false;
}