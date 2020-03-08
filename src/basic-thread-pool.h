/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* basic-thread-pool.h: a basic thread pool */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#ifndef BASIC_THREAD_POOL_H
#define BASIC_THREAD_POOL_H

#include <threads.h>

struct basic_thread_pool;
typedef struct basic_thread_pool basic_thread_pool_s;

basic_thread_pool_s *basic_thread_pool_new(size_t num_threads);

int basic_thread_pool_add(basic_thread_pool_s *pool, thrd_start_t func,
			  void *arg);

int basic_thread_pool_wait(basic_thread_pool_s *pool);

size_t basic_thread_pool_size(basic_thread_pool_s *pool);

void basic_thread_pool_stop_and_free(basic_thread_pool_s *pool);

#endif /* BASIC_THREAD_POOL_H */
