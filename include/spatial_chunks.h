// spatial_chunks.h - Chunk-based spatial organization for efficient particle updates
// Particles are organized into spatial cells for O(1) neighbor lookups

#ifndef SPATIAL_CHUNKS_H
#define SPATIAL_CHUNKS_H

#include <stdint.h>
#include <stdbool.h>
#include "particle_pool.h"

// Chunk configuration
#define CHUNK_SIZE 64.0f           // World units per chunk side
#define CHUNK_SIZE_INV (1.0f / CHUNK_SIZE)
#define MAX_CHUNKS_X 1024          // Max chunks in X direction
#define MAX_CHUNKS_Y 1024          // Max chunks in Y direction
#define MAX_CHUNKS (MAX_CHUNKS_X * MAX_CHUNKS_Y)

// Chunk ID encoding: combine x,y into single uint32
// Chunk ID = y * MAX_CHUNKS_X + x
typedef uint32_t ChunkId;

// Chunk state flags
typedef enum {
    CHUNK_FLAG_ACTIVE   = (1 << 0),  // Chunk has particles
    CHUNK_FLAG_DIRTY     = (1 << 1),  // Chunk needs update
    CHUNK_FLAG_BOUNDARY  = (1 << 2),  // Chunk touches black hole influence
} ChunkFlags;

// Particle in chunk linked list node
typedef struct ChunkParticleNode {
    uint32_t particle_index;
    uint32_t next_in_chunk;  // Next particle in this chunk (-1 if none)
} ChunkParticleNode;

// Spatial chunk
typedef struct {
    uint32_t first_particle;  // Index of first particle in chunk (-1 if empty)
    uint32_t particle_count;  // Number of particles in chunk
    uint16_t x, y;            // Chunk coordinates
    uint8_t flags;            // Chunk flags
} SpatialChunk;

// Spatial system
typedef struct SpatialSystem {
    SpatialChunk chunks[MAX_CHUNKS];
    ChunkParticleNode* particle_nodes;  // Per-particle chunk membership
    uint32_t max_chunks;
    uint32_t active_chunk_count;
    float chunk_size;
    float world_width;
    float world_height;
    uint32_t chunks_x;
    uint32_t chunks_y;
} SpatialSystem;

// Spatial system lifecycle
SpatialSystem* spatial_create(float world_width, float world_height, uint32_t max_particles);
void spatial_destroy(SpatialSystem* system);

// Chunk operations
ChunkId spatial_get_chunk_id(const SpatialSystem* system, float x, float y);
void spatial_insert(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx);
void spatial_remove(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx);
void spatial_move(SpatialSystem* system, ParticlePool* pool, uint32_t particle_idx, float new_x, float new_y);

// Chunk queries
SpatialChunk* spatial_get_chunk(SpatialSystem* system, ChunkId id);
SpatialChunk* spatial_get_chunk_at(SpatialSystem* system, float x, float y);
uint32_t* spatial_get_chunk_particles(SpatialSystem* system, ChunkId id, uint32_t* out_count);

// Iterator for chunk neighbor traversal
typedef struct ChunkIterator {
    SpatialSystem* system;
    int32_t dx, dy;       // Offset from center chunk
    int32_t cx, cy;       // Current chunk coords
    uint32_t current;     // Current particle in chunk
    uint32_t count;       // Remaining in current chunk
} ChunkIterator;

void spatial_iter_chunk_range(SpatialSystem* system, float center_x, float center_y,
                              float radius, ChunkIterator* iter_out);
bool spatial_iter_next(ChunkIterator* iter, uint32_t* particle_idx_out);

#endif // SPATIAL_CHUNKS_H