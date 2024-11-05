#pragma once

#include <stdbool.h>

// Basic spinlock
typedef bool slock_t;

// Initialize basic lock to an unlocked state
void slock_init(slock_t *l);

// Release basic lock
void slock_release(slock_t *l);

// Lock basic lock
// Spins until lock is locked
void slock_acquire(slock_t *l);

// Try locking basic lock
// If lock is unlocked, acquire it.
// Returns: true if was able to acquire lock
bool slock_try_acquire(slock_t *l);
