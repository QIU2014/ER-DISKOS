#ifndef ER_DISKOS_UEFI_PTHREAD_H
#define ER_DISKOS_UEFI_PTHREAD_H

#include <sys/types.h>
#include <time.h>

#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_MUTEX_RECURSIVE 1

static inline int uefi_sched_yield(void) {
    return 0;
}

static inline int uefi_pthread_attr_init(pthread_attr_t *attr) {
    (void)attr;
    return 0;
}

static inline int uefi_pthread_attr_setdetachstate(pthread_attr_t *attr, int state) {
    (void)attr;
    (void)state;
    return 0;
}

static inline int uefi_pthread_attr_destroy(pthread_attr_t *attr) {
    (void)attr;
    return 0;
}

static inline int uefi_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                      void *(*start_routine)(void *), void *arg) {
    (void)attr;
    (void)start_routine;
    (void)arg;
    *thread = 0;
    return 38;
}

static inline pthread_t uefi_pthread_self(void) {
    return 1;
}

static inline int uefi_pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

static inline int uefi_pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    (void)attr;
    (void)type;
    return 0;
}

static inline int uefi_pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

static inline int uefi_pthread_mutex_init(pthread_mutex_t *mutex,
                                          const pthread_mutexattr_t *attr) {
    (void)attr;
    (void)mutex;
    return 0;
}

static inline int uefi_pthread_mutex_destroy(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

static inline int uefi_pthread_mutex_lock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

static inline int uefi_pthread_mutex_trylock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

static inline int uefi_pthread_mutex_unlock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

static inline int uefi_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    (void)attr;
    (void)cond;
    return 0;
}

static inline int uefi_pthread_cond_destroy(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

static inline int uefi_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    (void)cond;
    (void)mutex;
    return 38;
}

static inline int uefi_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                              const struct timespec *timeout) {
    (void)cond;
    (void)mutex;
    (void)timeout;
    return 38;
}

static inline int uefi_pthread_cond_signal(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

static inline int uefi_pthread_cond_broadcast(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

#define pthread_attr_init uefi_pthread_attr_init
#define pthread_attr_setdetachstate uefi_pthread_attr_setdetachstate
#define pthread_attr_destroy uefi_pthread_attr_destroy
#define pthread_create uefi_pthread_create
#define pthread_self uefi_pthread_self
#define pthread_mutexattr_init uefi_pthread_mutexattr_init
#define pthread_mutexattr_settype uefi_pthread_mutexattr_settype
#define pthread_mutexattr_destroy uefi_pthread_mutexattr_destroy
#define pthread_mutex_init uefi_pthread_mutex_init
#define pthread_mutex_destroy uefi_pthread_mutex_destroy
#define pthread_mutex_lock uefi_pthread_mutex_lock
#define pthread_mutex_trylock uefi_pthread_mutex_trylock
#define pthread_mutex_unlock uefi_pthread_mutex_unlock
#define pthread_cond_init uefi_pthread_cond_init
#define pthread_cond_destroy uefi_pthread_cond_destroy
#define pthread_cond_wait uefi_pthread_cond_wait
#define pthread_cond_timedwait uefi_pthread_cond_timedwait
#define pthread_cond_signal uefi_pthread_cond_signal
#define pthread_cond_broadcast uefi_pthread_cond_broadcast
#define sched_yield uefi_sched_yield

#endif
