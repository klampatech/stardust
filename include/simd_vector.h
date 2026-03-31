// simd_vector.h - SIMD vector operations for particle system
// Provides SSE2/AVX2 vectorization for position/velocity arithmetic

#ifndef SIMD_VECTOR_H
#define SIMD_VECTOR_H

#include <stdint.h>
#include <stdbool.h>

// Detect best available SIMD instruction set
#if defined(__AVX2__)
    #define SIMD_LEVEL AVX2
    #include <immintrin.h>
#elif defined(__SSE2__)
    #define SIMD_LEVEL SSE2
    #include <emmintrin.h>
#else
    #define SIMD_LEVEL SCALAR
#endif

// 4-way float vector (SSE) or scalar fallback
typedef struct {
#if SIMD_LEVEL != SCALAR
    __m128 v;
#else
    float v[1];
#endif
} Vec4f;

// 2-way float vector (for x,y pairs)
typedef struct {
#if SIMD_LEVEL == AVX2
    __m256 v;
#elif SIMD_LEVEL == SSE2
    __m128 v;
#else
    float v[2];
#endif
} Vec2f;

// Initialize Vec2f from x,y values
static inline Vec2f vec2f_create(float x, float y) {
    Vec2f result;
#if SIMD_LEVEL == AVX2
    result.v = _mm256_set_ps(y, x, y, x, y, x, y, x);  // Note: reversed for _mm256_set_ps
#elif SIMD_LEVEL == SSE2
    result.v = _mm_set_ps(y, x, y, x);
#else
    result.v[0] = x;
    result.v[1] = y;
#endif
    return result;
}

// result = a + b (interleaved x,y pairs)
static inline Vec2f vec2f_add(Vec2f a, Vec2f b) {
    Vec2f result;
#if SIMD_LEVEL == AVX2
    result.v = _mm256_add_ps(a.v, b.v);
#elif SIMD_LEVEL == SSE2
    result.v = _mm_add_ps(a.v, b.v);
#else
    result.v[0] = a.v[0] + b.v[0];
    result.v[1] = a.v[1] + b.v[1];
#endif
    return result;
}

// result = a * scalar (interleaved x,y pairs, same scalar applied to both)
static inline Vec2f vec2f_scale(Vec2f a, float scalar) {
    Vec2f result;
#if SIMD_LEVEL == AVX2
    __m256 s = _mm256_set1_ps(scalar);
    result.v = _mm256_mul_ps(a.v, s);
#elif SIMD_LEVEL == SSE2
    __m128 s = _mm_set1_ps(scalar);
    result.v = _mm_mul_ps(a.v, s);
#else
    result.v[0] = a.v[0] * scalar;
    result.v[1] = a.v[1] * scalar;
#endif
    return result;
}

// Store Vec2f to float[2]
static inline void vec2f_store(Vec2f v, float* out) {
#if SIMD_LEVEL == AVX2
    _mm256_storeu_ps(out, v.v);
#elif SIMD_LEVEL == SSE2
    _mm_storeu_ps(out, v.v);
#else
    out[0] = v.v[0];
    out[1] = v.v[1];
#endif
}

// Load Vec2f from float[2]
static inline Vec2f vec2f_load(const float* src) {
    Vec2f result;
#if SIMD_LEVEL == AVX2
    result.v = _mm256_loadu_ps(src);
#elif SIMD_LEVEL == SSE2
    result.v = _mm_loadu_ps(src);
#else
    result.v[0] = src[0];
    result.v[1] = src[1];
#endif
    return result;
}

// Process 4 particles with SIMD: positions += velocities * dt
// positions: array of float[4] containing x values (next 4 contain y values)
// velocities: array of float[4] containing vx values (next 4 contain vy values)
// Assumes stride of 4 for accessing x or y components
static inline void simd_update_positions_4(float* pos_x, float* pos_y,
                                            const float* vel_x, const float* vel_y,
                                            float dt, uint32_t count) {
#if SIMD_LEVEL == SSE2 || SIMD_LEVEL == AVX2
    __m128 dt_vec = _mm_set1_ps(dt);
    uint32_t i = 0;

    // Process 4 particles at a time with SSE
    for (; i + 3 < count; i += 4) {
        __m128 px = _mm_loadu_ps(pos_x + i);
        __m128 py = _mm_loadu_ps(pos_y + i);
        __m128 vx = _mm_loadu_ps(vel_x + i);
        __m128 vy = _mm_loadu_ps(vel_y + i);

        // px += vx * dt
        px = _mm_add_ps(px, _mm_mul_ps(vx, dt_vec));
        py = _mm_add_ps(py, _mm_mul_ps(vy, dt_vec));

        _mm_storeu_ps(pos_x + i, px);
        _mm_storeu_ps(pos_y + i, py);
    }

    // Handle remaining particles
    for (; i < count; i++) {
        pos_x[i] += vel_x[i] * dt;
        pos_y[i] += vel_y[i] * dt;
    }
#else
    // Scalar fallback
    for (uint32_t i = 0; i < count; i++) {
        pos_x[i] += vel_x[i] * dt;
        pos_y[i] += vel_y[i] * dt;
    }
#endif
}

// Check if a chunk is within influence radius of a point
// Returns true if chunk center is within radius
static inline bool chunk_within_radius(float chunk_x, float chunk_y, float radius,
                                        float point_x, float point_y) {
    float dx = chunk_x - point_x;
    float dy = chunk_y - point_y;
    float dist_sq = dx * dx + dy * dy;
    return dist_sq <= radius * radius;
}

#endif // SIMD_VECTOR_H