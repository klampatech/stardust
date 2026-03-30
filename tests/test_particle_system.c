// test_particle_system.c - Unit tests for particle system

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "../include/particle_system.h"
#include "../include/material_types.h"
#include "../include/particle_pool.h"
#include "../include/spatial_chunks.h"

// Test helper macros
#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("FAIL: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
        return 1; \
    } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return 1; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, tol, msg) do { \
    if (fabsf((a) - (b)) > (tol)) { \
        printf("FAIL: %s (expected %f, got %f)\n", msg, (float)(b), (float)(a)); \
        return 1; \
    } \
} while(0)

static int test_material_types(void) {
    printf("Testing material types...\n");

    // Verify all 15 materials are defined
    ASSERT_EQ(MATERIAL_COUNT, 15, "Should have 15 material types");

    // Verify material flags
    ASSERT_TRUE(material_get_flags(MATERIAL_ROCK) & FLAG_DENSE, "Rock should be dense");
    ASSERT_TRUE(material_get_flags(MATERIAL_GAS) & FLAG_BUOYANT, "Gas should be buoyant");
    ASSERT_TRUE(material_get_flags(MATERIAL_PLASMA) & FLAG_CHARGED, "Plasma should be charged");
    ASSERT_TRUE(material_get_flags(MATERIAL_DARK_MATTER) & FLAG_CHARGED, "Dark matter should be charged");
    ASSERT_TRUE(material_get_flags(MATERIAL_DARK_MATTER) & FLAG_MAGNETIC, "Dark matter should be magnetic");

    // Verify material masses
    ASSERT_TRUE(material_get_mass(MATERIAL_METAL) > material_get_mass(MATERIAL_GAS), "Metal should be heavier than gas");

    printf("  PASS\n");
    return 0;
}

static int test_particle_pool(void) {
    printf("Testing particle pool...\n");

    ParticlePool* pool = pool_create(1000);
    ASSERT_TRUE(pool != NULL, "Pool should be created");

    // Initial state
    ASSERT_EQ(pool->active_count, 0, "Pool should start empty");
    ASSERT_EQ(pool->free_count, 1000, "Pool should have 1000 free slots");

    // Spawn particles
    Particle* p1 = pool_spawn(pool);
    ASSERT_TRUE(p1 != NULL, "Should spawn particle");
    ASSERT_EQ(pool->active_count, 1, "Should have 1 active particle");

    Particle* p2 = pool_spawn(pool);
    ASSERT_TRUE(p2 != NULL, "Should spawn second particle");
    ASSERT_EQ(pool->active_count, 2, "Should have 2 active particles");

    // Despawn
    pool_despawn(pool, p1);
    ASSERT_EQ(pool->active_count, 1, "Should have 1 active after despawn");

    // Re-spawn should reuse slot
    Particle* p3 = pool_spawn(pool);
    ASSERT_TRUE(p3 == p1, "Re-spawn should reuse first slot");

    // Bulk spawn
    Particle* batch = pool_spawnn(pool, 50);
    ASSERT_TRUE(batch != NULL, "Should bulk spawn");
    ASSERT_EQ(pool->active_count, 52, "Should have 52 active (p2 + p3 + 50 from spawnn)");

    // Despawn all
    pool_despawn_all(pool);
    ASSERT_EQ(pool->active_count, 0, "Should be empty after despawn all");
    ASSERT_EQ(pool->free_count, 1000, "All slots should be free");

    pool_destroy(pool);
    printf("  PASS\n");
    return 0;
}

static int test_spatial_chunks(void) {
    printf("Testing spatial chunks...\n");

    SpatialSystem* sys = spatial_create(1024.0f, 1024.0f, 1000);
    ASSERT_TRUE(sys != NULL, "Spatial system should be created");

    ParticlePool* pool = pool_create(100);
    ASSERT_TRUE(pool != NULL, "Pool should be created");

    // Spawn particles at known positions
    Particle* p1 = pool_spawn(pool);
    uint32_t p1_idx = (uint32_t)(p1 - pool->particles);
    p1->x = 100.0f; p1->y = 100.0f;
    spatial_insert(sys, pool, p1_idx);

    ChunkId cid1 = p1->chunk_id;

    // Verify particle is in correct chunk
    SpatialChunk* chunk = spatial_get_chunk(sys, cid1);
    ASSERT_TRUE(chunk != NULL, "Chunk should exist");
    ASSERT_EQ(chunk->particle_count, 1, "Chunk should have 1 particle");

    // Spawn another particle in same chunk
    Particle* p2 = pool_spawn(pool);
    uint32_t p2_idx = (uint32_t)(p2 - pool->particles);
    p2->x = 110.0f; p2->y = 110.0f;
    spatial_insert(sys, pool, p2_idx);  // Insert p2 into spatial system

    ASSERT_EQ(chunk->particle_count, 2, "Chunk should have 2 particles");

    // Move particle to different chunk
    ChunkId cid_old = cid1;  // Save old chunk id before move
    spatial_move(sys, pool, p1_idx, 500.0f, 500.0f);

    // Re-fetch old chunk after move (pointer was stale)
    SpatialChunk* old_chunk = spatial_get_chunk(sys, cid_old);
    ASSERT_EQ(old_chunk->particle_count, 1, "Original chunk should have 1 particle after move");

    // Clean up
    spatial_destroy(sys);
    pool_destroy(pool);
    printf("  PASS\n");
    return 0;
}

static int test_particle_system(void) {
    printf("Testing particle system...\n");

    ParticleSystem* sys = particle_system_create(1024.0f, 1024.0f, 50000);
    ASSERT_TRUE(sys != NULL, "Particle system should be created");
    ASSERT_EQ(sys->pool->max_particles, 50000, "Should support 50k particles");

    // Register a template
    SpawnTemplate rock_template = {
        .material = MATERIAL_ROCK,
        .spread_radius = 10.0f,
        .velocity_min = 1.0f,
        .velocity_max = 5.0f,
        .mass_modifier = 1.0f,
        .base_lifetime = 10000
    };
    int template_id = particle_system_register_template(sys, &rock_template);
    ASSERT_TRUE(template_id >= 0, "Template should be registered");

    // Spawn a cluster
    uint32_t spawned = particle_system_spawn_cluster(sys, 512.0f, 512.0f, template_id, 1000);
    ASSERT_EQ(spawned, 1000, "Should spawn 1000 particles");
    ASSERT_EQ(particle_system_get_count(sys), 1000, "Should have 1000 particles");

    // Update
    particle_system_update(sys, 0.016f);

    // Get buffer
    particle_system_prepare_render_buffer(sys);
    const ParticleBuffer* buf = particle_system_get_buffer(sys);
    ASSERT_TRUE(buf != NULL, "Buffer should be accessible");
    ASSERT_EQ(buf->count, 1000, "Buffer should have 1000 particles");

    // Despawn all
    particle_system_despawn_all(sys);
    ASSERT_EQ(particle_system_get_count(sys), 0, "Should be empty after despawn all");

    particle_system_destroy(sys);
    printf("  PASS\n");
    return 0;
}

static int test_50k_particles(void) {
    printf("Testing 50k particle capacity...\n");

    ParticleSystem* sys = particle_system_create(2048.0f, 2048.0f, 65536);
    ASSERT_TRUE(sys != NULL, "Particle system should be created");

    SpawnTemplate templates[5] = {
        {MATERIAL_ROCK, 5.0f, 0.5f, 2.0f, 1.0f, 10000},
        {MATERIAL_ICE, 5.0f, 0.5f, 2.0f, 0.9f, 8000},
        {MATERIAL_METAL, 5.0f, 0.5f, 2.0f, 2.5f, 15000},
        {MATERIAL_GAS, 10.0f, 0.1f, 1.0f, 0.01f, 5000},
        {MATERIAL_DUST, 3.0f, 0.2f, 1.5f, 0.05f, 7000},
    };

    for (int i = 0; i < 5; i++) {
        int tid = particle_system_register_template(sys, &templates[i]);
        ASSERT_TRUE(tid >= 0, "Template should register");
    }

    // Spawn 50k particles across different materials
    uint32_t total = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t count = particle_system_spawn_cluster(sys, 1024.0f, 1024.0f, i, 10000);
        total += count;
    }

    ASSERT_EQ(total, 50000, "Should spawn exactly 50k particles");
    ASSERT_EQ(particle_system_get_count(sys), 50000, "System should report 50k active");

    // Update and verify no crashes
    for (int i = 0; i < 100; i++) {
        particle_system_update(sys, 0.016f);
        particle_system_prepare_render_buffer(sys);
    }

    particle_system_destroy(sys);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    printf("\n=== Particle System Tests ===\n\n");

    int result = 0;
    result |= test_material_types();
    result |= test_particle_pool();
    result |= test_spatial_chunks();
    result |= test_particle_system();
    result |= test_50k_particles();

    printf("\n%s: %s\n", result == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           result == 0 ? "OK" : "FAILURES");
    return result;
}