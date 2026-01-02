#ifndef NUMSTORE_CORE_ADPTV_CLCK_ALLOC_H
#define NUMSTORE_CORE_ADPTV_CLCK_ALLOC_H

#include <numstore/core/error.h>
#include <numstore/core/spx_latch.h>
#include <numstore/intf/types.h>

/**
 * @file adptv_clck_alloc.h
 * @brief Adaptive clock allocator with dynamic capacity adjustment
 *
 * This allocator dynamically grows and shrinks based on usage, similar to
 * adptv_hash_table. It uses two clock allocators during resizing and
 * incrementally migrates elements. Returns integer handles instead of
 * pointers since data location may change during migration.
 */

/**
 * Settings for adaptive clock allocator behavior
 */
struct adptv_clck_alloc_settings {
  /**
   * Number of elements to migrate per allocation operation.
   * Higher values complete migration faster but increase per-operation latency.
   */
  u32 migration_work;

  /**
   * Maximum capacity (number of elements). Allocator will not grow beyond this.
   */
  u32 max_capacity;

  /**
   * Minimum capacity (number of elements). Allocator will not shrink below this.
   */
  u32 min_capacity;
};

/**
 * Default settings for adaptive clock allocator
 */
#define ADPTV_CLCK_ALLOC_DEFAULT_SETTINGS                                      \
  (struct adptv_clck_alloc_settings) {                                         \
    .migration_work = 4, .max_capacity = 1 << 20, /* 1M elements */            \
    .min_capacity = 16,                                                        \
  }

/**
 * Adaptive clock allocator structure (opaque)
 */
struct adptv_clck_alloc;

/**
 * Initialize an adaptive clock allocator
 *
 * @param aca Pointer to allocator structure
 * @param elem_size Size of each element in bytes
 * @param initial_capacity Initial number of elements
 * @param settings Configuration settings
 * @param e Error context
 * @return 0 on success, error code on failure
 */
int adptv_clck_alloc_init(struct adptv_clck_alloc **aca, size_t elem_size,
                          u32 initial_capacity,
                          struct adptv_clck_alloc_settings settings,
                          error *e);

/**
 * Allocate an element and return its handle
 *
 * This operation may trigger:
 * - Incremental migration if resizing is in progress
 * - Capacity doubling if allocator is full
 *
 * @param aca Allocator
 * @param e Error context
 * @return Non-negative handle on success, -1 on failure (see error context)
 */
i32 adptv_clck_alloc_alloc(struct adptv_clck_alloc *aca, error *e);

/**
 * Allocate and zero-initialize an element
 *
 * @param aca Allocator
 * @param e Error context
 * @return Non-negative handle on success, -1 on failure
 */
i32 adptv_clck_alloc_calloc(struct adptv_clck_alloc *aca, error *e);

/**
 * Free an element by its handle
 *
 * This operation may trigger:
 * - Capacity halving if allocator is 25% full or less
 *
 * @param aca Allocator
 * @param handle Element handle returned from alloc/calloc
 * @param e Error context
 */
void adptv_clck_alloc_free(struct adptv_clck_alloc *aca, i32 handle,
                           error *e);

/**
 * Get pointer to element data from handle
 *
 * WARNING: Returned pointer is only valid until the next alloc/free operation,
 * as these operations may trigger migration that relocates elements.
 *
 * @param aca Allocator
 * @param handle Element handle
 * @return Pointer to element data, or NULL if handle is invalid
 */
void *adptv_clck_alloc_get_at(struct adptv_clck_alloc *aca, i32 handle);

/**
 * Get current number of allocated elements
 *
 * @param aca Allocator
 * @return Number of allocated elements
 */
u32 adptv_clck_alloc_size(const struct adptv_clck_alloc *aca);

/**
 * Get current capacity
 *
 * @param aca Allocator
 * @return Current capacity (number of elements)
 */
u32 adptv_clck_alloc_capacity(const struct adptv_clck_alloc *aca);

/**
 * Clean up and free allocator resources
 *
 * @param aca Allocator to destroy
 */
void adptv_clck_alloc_free_allocator(struct adptv_clck_alloc *aca);

#endif // NUMSTORE_CORE_ADPTV_CLCK_ALLOC_H
