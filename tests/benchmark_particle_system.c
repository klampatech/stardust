// benchmark_particle_system.c - Performance benchmark for particle system
// Measures update time for 50k particles to verify < 10ms requirement

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../include/particle_system.h"
#include "../include/material_types.h"

static double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(void) {
    printf("\n=== Particle System Performance Benchmark ===\n\n");

    const uint32_t CAPACITY = 65536;
    const uint32_t WORLD_SIZE = 2048;

    printf("Creating particle system with %u max particles...\n", CAPACITY);
    ParticleSystem* sys = particle_system_create(WORLD_SIZE, WORLD_SIZE, CAPACITY);
    if (!sys) {
        printf("ERROR: Failed to create particle system\n");
        return 1;
    }

    // Register templates for different materials
    SpawnTemplate templates[5] = {
        {MATERIAL_ROCK, 10.0f, 0.5f, 2.0f, 1.0f, 100000},
        {MATERIAL_ICE, 10.0f, 0.5f, 2.0f, 0.9f, 80000},
        {MATERIAL_METAL, 10.0f, 0.5f, 2.0f, 2.5f, 150000},
        {MATERIAL_GAS, 20.0f, 0.1f, 1.0f, 0.01f, 50000},
        {MATERIAL_DUST, 8.0f, 0.2f, 1.5f, 0.05f, 70000},
    };

    for (int i = 0; i < 5; i++) {
        particle_system_register_template(sys, &templates[i]);
    }

    // Spawn 50k particles across different materials
    printf("Spawning 50,000 particles...\n");
    uint32_t total = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t count = particle_system_spawn_cluster(sys, WORLD_SIZE/2.0f, WORLD_SIZE/2.0f, i, 10000);
        total += count;
    }
    printf("Spawned %u particles\n", total);

    // Set black hole at center with large influence radius
    particle_system_set_black_hole(sys, WORLD_SIZE/2.0f, WORLD_SIZE/2.0f, 256.0f);

    // Warm up
    printf("Warming up (10 frames)...\n");
    for (int i = 0; i < 10; i++) {
        particle_system_update(sys, 0.016f);
    }

    // Benchmark: Run 100 frames and measure average update time
    printf("Running benchmark (100 frames)...\n");
    double total_time = 0.0;
    float dt = 0.016f;  // ~60fps

    for (int i = 0; i < 100; i++) {
        double start = get_time_seconds();
        particle_system_update(sys, dt);
        double end = get_time_seconds();
        total_time += (end - start);
    }

    double avg_time_ms = (total_time / 100.0) * 1000.0;

    // Get stats
    ParticleSystemStats stats;
    particle_system_get_stats(sys, &stats);

    printf("\n=== Benchmark Results ===\n");
    printf("Active particles: %u\n", stats.active_particles);
    printf("Average update time: %.3f ms\n", avg_time_ms);
    printf("Particles per frame: %u\n", stats.particles_per_frame);
    printf("Active chunks: %u\n", stats.active_chunks);
    printf("\n=== Performance Target ===\n");
    printf("Target: < 10ms per frame\n");
    printf("Result: %.3f ms\n", avg_time_ms);
    if (avg_time_ms < 10.0) {
        printf("STATUS: PASS (%.1f%% under target)\n", (10.0 - avg_time_ms) / 10.0 * 100.0);
    } else {
        printf("STATUS: FAIL (%.1f%% over target)\n", (avg_time_ms - 10.0) / 10.0 * 100.0);
    }

    // Test benchmark mode
    printf("\n=== Benchmark Mode Test ===\n");
    particle_system_enable_benchmark(sys, true);
    printf("Benchmark mode enabled\n");

    particle_system_update(sys, dt);
    particle_system_get_stats(sys, &stats);
    printf("With benchmark mode - update time: %.3f ms\n", stats.update_time_ms);

    particle_system_enable_benchmark(sys, false);
    printf("Benchmark mode disabled\n");

    // Cleanup
    particle_system_destroy(sys);
    printf("\nBenchmark complete.\n");
    return 0;
}