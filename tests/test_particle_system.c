// test_particle_system.c - Unit tests for particle system

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "../include/particle_system.h"
#include "../include/material_types.h"
#include "../include/particle_pool.h"
#include "../include/spatial_chunks.h"
#include "../include/black_hole.h"

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

    SpatialSystem* sys = spatial_create(1024.0f, 1024.0f, 1000, 0.0f);  // 0 = use default chunk size
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

static int test_spatial_chunk_configurable(void) {
    printf("Testing configurable spatial chunks...\n");

    // Test default chunk size (0.0f should use CHUNK_SIZE = 64.0f)
    SpatialSystem* sys1 = spatial_create(1024.0f, 1024.0f, 1000, 0.0f);
    ASSERT_TRUE(sys1 != NULL, "System with default chunk size should be created");
    ASSERT_FLOAT_EQ(spatial_get_chunk_size(sys1), 64.0f, 0.001f, "Default chunk size should be 64.0f");

    uint32_t cx1, cy1;
    spatial_get_dimensions(sys1, &cx1, &cy1);
    ASSERT_EQ(cx1, 16u, "1024/64 = 16 chunks in X");  // 1024 / 64 = 16
    ASSERT_EQ(cy1, 16u, "1024/64 = 16 chunks in Y");
    ASSERT_EQ(spatial_get_chunk_count(sys1), 256u, "Should have 16*16 = 256 chunks");
    spatial_destroy(sys1);

    // Test custom chunk size (128.0f)
    SpatialSystem* sys2 = spatial_create(1024.0f, 1024.0f, 1000, 128.0f);
    ASSERT_TRUE(sys2 != NULL, "System with custom chunk size should be created");
    ASSERT_FLOAT_EQ(spatial_get_chunk_size(sys2), 128.0f, 0.001f, "Custom chunk size should be 128.0f");

    uint32_t cx2, cy2;
    spatial_get_dimensions(sys2, &cx2, &cy2);
    ASSERT_EQ(cx2, 8u, "1024/128 = 8 chunks in X");  // 1024 / 128 = 8
    ASSERT_EQ(cy2, 8u, "1024/128 = 8 chunks in Y");
    ASSERT_EQ(spatial_get_chunk_count(sys2), 64u, "Should have 8*8 = 64 chunks");
    spatial_destroy(sys2);

    // Test small chunk size (32.0f) for finer granularity
    SpatialSystem* sys3 = spatial_create(1024.0f, 1024.0f, 1000, 32.0f);
    ASSERT_TRUE(sys3 != NULL, "System with small chunk size should be created");
    ASSERT_FLOAT_EQ(spatial_get_chunk_size(sys3), 32.0f, 0.001f, "Small chunk size should be 32.0f");

    uint32_t cx3, cy3;
    spatial_get_dimensions(sys3, &cx3, &cy3);
    ASSERT_EQ(cx3, 32u, "1024/32 = 32 chunks in X");  // 1024 / 32 = 32
    ASSERT_EQ(cy3, 32u, "1024/32 = 32 chunks in Y");
    ASSERT_EQ(spatial_get_chunk_count(sys3), 1024u, "Should have 32*32 = 1024 chunks");
    spatial_destroy(sys3);

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

static int test_black_hole_create(void) {
    printf("Testing black hole creation...\n");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(512.0f, 512.0f, &config);
    ASSERT_TRUE(bh != NULL, "Black hole should be created");

    float x, y;
    black_hole_get_position(bh, &x, &y);
    ASSERT_FLOAT_EQ(x, 512.0f, 0.001f, "Black hole X position should be 512");
    ASSERT_FLOAT_EQ(y, 512.0f, 0.001f, "Black hole Y position should be 512");

    BlackHoleConfig cfg = black_hole_get_config(bh);
    ASSERT_FLOAT_EQ(cfg.mass, 1000.0f, 0.001f, "Mass should be 1000");
    ASSERT_FLOAT_EQ(cfg.destruction_radius, 10.0f, 0.001f, "Destruction radius should be 10");
    ASSERT_FLOAT_EQ(cfg.influence_radius, 200.0f, 0.001f, "Influence radius should be 200");

    black_hole_destroy(bh);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_destruction_radius(void) {
    printf("Testing black hole destruction radius...\n");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // Inside destruction radius
    ASSERT_TRUE(black_hole_is_within_destruction_radius(bh, 100.0f, 100.0f), "Center should be in destruction radius");
    ASSERT_TRUE(black_hole_is_within_destruction_radius(bh, 105.0f, 100.0f), "Point at 5 units should be in destruction radius");
    ASSERT_TRUE(black_hole_is_within_destruction_radius(bh, 100.0f, 108.0f), "Point at 8 units should be in destruction radius");

    // Outside destruction radius
    ASSERT_TRUE(!black_hole_is_within_destruction_radius(bh, 120.0f, 100.0f), "Point at 20 units should be outside destruction radius");
    ASSERT_TRUE(!black_hole_is_within_destruction_radius(bh, 100.0f, 130.0f), "Point at 30 units should be outside destruction radius");

    black_hole_destroy(bh);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_influence_radius(void) {
    printf("Testing black hole influence radius...\n");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // Inside influence radius
    ASSERT_TRUE(black_hole_is_within_influence_radius(bh, 100.0f, 100.0f), "Center should be in influence radius");
    ASSERT_TRUE(black_hole_is_within_influence_radius(bh, 150.0f, 100.0f), "Point at 50 units should be in influence radius");
    ASSERT_TRUE(black_hole_is_within_influence_radius(bh, 250.0f, 100.0f), "Point at 150 units should be in influence radius");

    // Outside influence radius
    ASSERT_TRUE(!black_hole_is_within_influence_radius(bh, 350.0f, 100.0f), "Point at 250 units should be outside influence radius");

    black_hole_destroy(bh);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_tidal_factor(void) {
    printf("Testing black hole tidal factor...\n");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // At center (inside destruction radius) - maximum tidal
    float tidal = black_hole_calculate_tidal_factor(bh, 100.0f, 100.0f);
    ASSERT_FLOAT_EQ(tidal, 5.0f, 0.001f, "Tidal at center should be 5.0");

    // At destruction radius edge - high tidal
    tidal = black_hole_calculate_tidal_factor(bh, 110.0f, 100.0f);
    ASSERT_TRUE(tidal > 1.0f && tidal <= 5.0f, "Tidal near destruction radius should be > 1.0");

    // At influence radius edge - normal tidal
    tidal = black_hole_calculate_tidal_factor(bh, 300.0f, 100.0f);
    ASSERT_FLOAT_EQ(tidal, 1.0f, 0.001f, "Tidal at influence edge should be 1.0");

    // Outside influence - normal tidal
    tidal = black_hole_calculate_tidal_factor(bh, 400.0f, 100.0f);
    ASSERT_FLOAT_EQ(tidal, 1.0f, 0.001f, "Tidal outside influence should be 1.0");

    black_hole_destroy(bh);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_gravity_particle_destruction(void) {
    printf("Testing black hole particle destruction...\n");

    ParticlePool* pool = pool_create(100);
    ASSERT_TRUE(pool != NULL, "Pool should be created");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 20.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // Spawn particle inside destruction radius
    Particle* p = pool_spawn(pool);
    ASSERT_TRUE(p != NULL, "Should spawn particle");

    // Place particle inside event horizon (at center)
    p->x = 100.0f;
    p->y = 100.0f;
    p->vx = 0.0f;
    p->vy = 0.0f;
    p->mass = 1.0f;
    p->lifetime = 1000;

    // Apply gravity
    uint32_t destroyed = black_hole_apply_gravity(bh, pool, 0.016f);

    ASSERT_EQ(destroyed, 1u, "Should have destroyed 1 particle");
    ASSERT_EQ(pool->active_count, 0u, "Pool should be empty after destruction");

    // Check stats
    BlackHoleStats stats;
    black_hole_get_stats(bh, &stats);
    ASSERT_EQ(stats.particles_consumed, 1u, "Should have consumed 1 particle");
    ASSERT_FLOAT_EQ(stats.total_mass_consumed, 1.0f, 0.001f, "Should have consumed 1.0 mass");

    black_hole_destroy(bh);
    pool_destroy(pool);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_gravity_particle_attraction(void) {
    printf("Testing black hole gravitational attraction...\n");

    ParticlePool* pool = pool_create(100);
    ASSERT_TRUE(pool != NULL, "Pool should be created");

    BlackHoleConfig config = {
        .mass = 10000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // Spawn particle at distance (inside influence, outside destruction)
    Particle* p = pool_spawn(pool);
    ASSERT_TRUE(p != NULL, "Should spawn particle");

    // Place particle at 100 units away, stationary
    p->x = 200.0f;
    p->y = 100.0f;
    p->vx = 0.0f;
    p->vy = 0.0f;
    p->mass = 1.0f;
    p->lifetime = 1000;

    float initial_vx = p->vx;
    float initial_vy = p->vy;

    // Apply gravity
    uint32_t destroyed = black_hole_apply_gravity(bh, pool, 0.016f);

    ASSERT_EQ(destroyed, 0u, "Should not have destroyed particle");
    ASSERT_TRUE(p->vx != initial_vx || p->vy != initial_vy, "Particle velocity should change");

    black_hole_destroy(bh);
    pool_destroy(pool);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_particle_system_integration(void) {
    printf("Testing black hole particle system integration...\n");

    ParticleSystem* sys = particle_system_create(1024.0f, 1024.0f, 1000);
    ASSERT_TRUE(sys != NULL, "Particle system should be created");

    // Create and set black hole
    BlackHoleConfig config = {
        .mass = 5000.0f,
        .destruction_radius = 30.0f,
        .influence_radius = 300.0f
    };

    BlackHole* bh = black_hole_create(512.0f, 512.0f, &config);
    particle_system_set_black_hole(sys, bh);

    // Verify black hole is set
    ASSERT_TRUE(particle_system_get_black_hole(sys) == bh, "Black hole should be retrievable");

    // Register a template and spawn particles
    SpawnTemplate template = {
        .material = MATERIAL_ROCK,
        .spread_radius = 5.0f,
        .velocity_min = 0.0f,
        .velocity_max = 0.0f,
        .mass_modifier = 1.0f,
        .base_lifetime = 10000
    };

    int template_id = particle_system_register_template(sys, &template);
    ASSERT_TRUE(template_id >= 0, "Template should be registered");

    // Spawn particles near black hole (some will be destroyed immediately)
    uint32_t spawned = particle_system_spawn_cluster(sys, 512.0f, 512.0f, template_id, 100);
    ASSERT_TRUE(spawned > 0, "Should spawn some particles");

    // Update (black hole will pull and destroy some)
    for (int i = 0; i < 10; i++) {
        particle_system_update(sys, 0.016f);
    }

    // Get black hole stats
    const BlackHoleStats* stats = particle_system_get_black_hole_stats(sys);
    ASSERT_TRUE(stats != NULL, "Stats should be retrievable");

    particle_system_destroy(sys);
    printf("  PASS\n");
    return 0;
}

static int test_black_hole_stats_reset(void) {
    printf("Testing black hole stats reset...\n");

    BlackHoleConfig config = {
        .mass = 1000.0f,
        .destruction_radius = 10.0f,
        .influence_radius = 200.0f
    };

    BlackHole* bh = black_hole_create(100.0f, 100.0f, &config);

    // Add some consumed mass/particles
    bh->particles_consumed = 10;
    bh->total_mass_consumed = 50.0f;

    // Reset stats
    black_hole_reset_stats(bh);

    BlackHoleStats stats;
    black_hole_get_stats(bh, &stats);
    ASSERT_EQ(stats.particles_consumed, 0u, "Particles consumed should be 0 after reset");
    ASSERT_FLOAT_EQ(stats.total_mass_consumed, 0.0f, 0.001f, "Total mass consumed should be 0 after reset");

    black_hole_destroy(bh);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    printf("\n=== Particle System Tests ===\n\n");

    int result = 0;
    result |= test_material_types();
    result |= test_particle_pool();
    result |= test_spatial_chunks();
    result |= test_spatial_chunk_configurable();
    result |= test_particle_system();
    result |= test_50k_particles();
    result |= test_black_hole_create();
    result |= test_black_hole_destruction_radius();
    result |= test_black_hole_influence_radius();
    result |= test_black_hole_tidal_factor();
    result |= test_black_hole_gravity_particle_destruction();
    result |= test_black_hole_gravity_particle_attraction();
    result |= test_black_hole_particle_system_integration();
    result |= test_black_hole_stats_reset();

    printf("\n%s: %s\n", result == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           result == 0 ? "OK" : "FAILURES");
    return result;
}