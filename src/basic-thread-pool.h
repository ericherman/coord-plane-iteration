/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* basic-thread-pool.h: a basic thread pool */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef BASIC_THREAD_POOL_H
#define BASIC_THREAD_POOL_H

#include <threads.h>

struct basic_thread_pool;

struct basic_thread_pool *basic_thread_pool_new(size_t num_threads);

int basic_thread_pool_add(struct basic_thread_pool *pool, thrd_start_t func,
			  void *arg);

int basic_thread_pool_wait(struct basic_thread_pool *pool);

size_t basic_thread_pool_size(struct basic_thread_pool *pool);

size_t basic_thread_pool_queue_size(struct basic_thread_pool *pool);

/* discards any unfinished work */
void basic_thread_pool_stop_and_free(struct basic_thread_pool **pool_ref);

#endif /* BASIC_THREAD_POOL_H */
