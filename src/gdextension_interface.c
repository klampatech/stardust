// gdextension_interface.c - GDExtension C interface implementation

#include <stddef.h>
#include "gdextension_interface.h"

ParticleSystemHandle gd_particle_system_create(float world_width, float world_height, uint32_t max_particles) {
    ParticleSystem* system = particle_system_create(world_width, world_height, max_particles);
    return (ParticleSystemHandle)system;
}

void gd_particle_system_destroy(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_destroy((ParticleSystem*)handle);
    }
}

int gd_particle_system_register_template(ParticleSystemHandle handle,
                                         uint8_t material,
                                         float spread_radius,
                                         float velocity_min,
                                         float velocity_max,
                                         float mass_modifier,
                                         uint32_t base_lifetime) {
    if (!handle) return -1;

    SpawnTemplate tmpl;
    tmpl.material = material;
    tmpl.spread_radius = spread_radius;
    tmpl.velocity_min = velocity_min;
    tmpl.velocity_max = velocity_max;
    tmpl.mass_modifier = mass_modifier;
    tmpl.base_lifetime = base_lifetime;

    return particle_system_register_template((ParticleSystem*)handle, &tmpl);
}

uint32_t gd_particle_system_spawn_cluster(ParticleSystemHandle handle,
                                           float pos_x, float pos_y,
                                           int template_id,
                                           uint32_t count) {
    if (!handle) return 0;
    return particle_system_spawn_cluster((ParticleSystem*)handle, pos_x, pos_y, template_id, count);
}

void gd_particle_system_despawn(ParticleSystemHandle handle, uint32_t particle_id) {
    if (handle) {
        particle_system_despawn_particle((ParticleSystem*)handle, particle_id);
    }
}

void gd_particle_system_despawn_all(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_despawn_all((ParticleSystem*)handle);
    }
}

void gd_particle_system_update(ParticleSystemHandle handle, float delta) {
    if (handle) {
        particle_system_update((ParticleSystem*)handle, delta);
    }
}

void gd_particle_system_prepare_buffer(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_prepare_render_buffer((ParticleSystem*)handle);
    }
}

const ParticleBuffer* gd_particle_system_get_buffer(ParticleSystemHandle handle) {
    if (!handle) return NULL;
    return particle_system_get_buffer((ParticleSystem*)handle);
}

uint32_t gd_particle_system_get_count(ParticleSystemHandle handle) {
    if (!handle) return 0;
    return particle_system_get_count((ParticleSystem*)handle);
}

bool gd_particle_system_get_particle(ParticleSystemHandle handle, uint32_t particle_id,
                                      float* out_x, float* out_y,
                                      float* out_vx, float* out_vy,
                                      uint8_t* out_material,
                                      float* out_mass, uint32_t* out_lifetime) {
    if (!handle) return false;
    return particle_system_get_particle((ParticleSystem*)handle, particle_id,
                                         out_x, out_y, out_vx, out_vy,
                                         out_material, out_mass, out_lifetime);
}

void gd_particle_system_get_stats(ParticleSystemHandle handle, ParticleSystemStats* out_stats) {
    if (handle && out_stats) {
        particle_system_get_stats((ParticleSystem*)handle, out_stats);
    }
}

void gd_particle_system_pause(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_pause((ParticleSystem*)handle);
    }
}

void gd_particle_system_resume(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_resume((ParticleSystem*)handle);
    }
}

bool gd_particle_system_is_paused(ParticleSystemHandle handle) {
    if (!handle) return false;
    return particle_system_is_paused((ParticleSystem*)handle);
}

void gd_particle_system_step(ParticleSystemHandle handle, float dt) {
    if (handle) {
        particle_system_step((ParticleSystem*)handle, dt);
    }
}

void gd_particle_system_set_black_hole(ParticleSystemHandle handle,
                                        float mass, float radius, float influence,
                                        float pos_x, float pos_y) {
    if (handle) {
        particle_system_set_black_hole((ParticleSystem*)handle, mass, radius, influence, pos_x, pos_y);
    }
}

void gd_particle_system_get_black_hole(ParticleSystemHandle handle, BlackHoleConfig* out_config) {
    if (handle) {
        particle_system_get_black_hole((ParticleSystem*)handle, out_config);
    }
}

void gd_particle_system_get_black_hole_stats(ParticleSystemHandle handle, BlackHoleStats* out_stats) {
    if (handle) {
        particle_system_get_black_hole_stats((ParticleSystem*)handle, out_stats);
    }
}

void gd_particle_system_clear_black_hole_stats(ParticleSystemHandle handle) {
    if (handle) {
        particle_system_clear_black_hole_stats((ParticleSystem*)handle);
    }
}

uint32_t gd_particle_system_get_chunk_count(ParticleSystemHandle handle) {
    if (!handle) return 0;
    return particle_system_get_chunk_count((ParticleSystem*)handle);
}

uint32_t gd_particle_system_get_active_chunks(ParticleSystemHandle handle, uint32_t* out_chunk_ids, uint32_t max_chunks) {
    if (!handle) return 0;
    return particle_system_get_active_chunks((ParticleSystem*)handle, out_chunk_ids, max_chunks);
}