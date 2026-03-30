// gdextension_interface.h - Godot GDExtension C interface for particle system
// This header provides the bindings for Godot to interact with the particle engine

#ifndef GDEXTENSION_INTERFACE_H
#define GDEXTENSION_INTERFACE_H

#include "particle_system.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to particle system instance
typedef void* ParticleSystemHandle;

// Lifecycle (call from Godot _init / _ready)
ParticleSystemHandle gd_particle_system_create(float world_width, float world_height, uint32_t max_particles);
void gd_particle_system_destroy(ParticleSystemHandle handle);

// Template management
int gd_particle_system_register_template(ParticleSystemHandle handle,
                                          uint8_t material,
                                          float spread_radius,
                                          float velocity_min,
                                          float velocity_max,
                                          float mass_modifier,
                                          uint32_t base_lifetime);

// Spawning
uint32_t gd_particle_system_spawn_cluster(ParticleSystemHandle handle,
                                           float pos_x, float pos_y,
                                           int template_id,
                                           uint32_t count);
void gd_particle_system_despawn(ParticleSystemHandle handle, uint32_t particle_id);
void gd_particle_system_despawn_all(ParticleSystemHandle handle);

// Per-frame update (call from Godot physics process)
void gd_particle_system_update(ParticleSystemHandle handle, float delta);

// Render buffer access (call from Godot render thread)
void gd_particle_system_prepare_buffer(ParticleSystemHandle handle);

// Get read-only buffer pointer (valid until next gd_* call)
const ParticleBuffer* gd_particle_system_get_buffer(ParticleSystemHandle handle);

// Query (for Godot scripts to query particle data)
uint32_t gd_particle_system_get_count(ParticleSystemHandle handle);

bool gd_particle_system_get_particle(ParticleSystemHandle handle, uint32_t particle_id,
                                      float* out_x, float* out_y,
                                      float* out_vx, float* out_vy,
                                      uint8_t* out_material,
                                      float* out_mass, uint32_t* out_lifetime);

// Stats (for debugging/monitoring)
void gd_particle_system_get_stats(ParticleSystemHandle handle, ParticleSystemStats* out_stats);

// Exported function names for GDExtension registration
#define GD_PARTICLE_CREATE "gd_particle_system_create"
#define GD_PARTICLE_DESTROY "gd_particle_system_destroy"
#define GD_PARTICLE_REGISTER_TEMPLATE "gd_particle_system_register_template"
#define GD_PARTICLE_SPAWN_CLUSTER "gd_particle_system_spawn_cluster"
#define GD_PARTICLE_DESPWAN "gd_particle_system_despawn"
#define GD_PARTICLE_DESPWAN_ALL "gd_particle_system_despawn_all"
#define GD_PARTICLE_UPDATE "gd_particle_system_update"
#define GD_PARTICLE_PREPARE_BUFFER "gd_particle_system_prepare_buffer"
#define GD_PARTICLE_GET_BUFFER "gd_particle_system_get_buffer"
#define GD_PARTICLE_GET_COUNT "gd_particle_system_get_count"
#define GD_PARTICLE_GET_PARTICLE "gd_particle_system_get_particle"
#define GD_PARTICLE_GET_STATS "gd_particle_system_get_stats"

#ifdef __cplusplus
}
#endif

#endif // GDEXTENSION_INTERFACE_H