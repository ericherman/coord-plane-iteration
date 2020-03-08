/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* basic-thread-pool.c: a basic thread pool */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <logerr-die.h>
#include <alloc-or-die.h>

#include <basic-thread-pool.h>

/* Thread command, check return code, and whine if not success */
#ifndef Tc
#define Tc(thrd_call, our_id) do { \
	int _thrd_err = (thrd_call); \
	switch (_thrd_err) { \
	case thrd_success: \
		break; \
	case thrd_nomem: \
		logerror("%s for %zu: thrd_nomem", #thrd_call, our_id); \
		break; \
	case thrd_error: \
		logerror("%s for %zu: thrd_error", #thrd_call, our_id); \
		break; \
	case thrd_timedout: \
		logerror("%s for %zu: thrd_timedout", #thrd_call, our_id); \
		break; \
	case thrd_busy: \
		logerror("%s for %zu: thrd_busy", #thrd_call, our_id); \
		break; \
	default: \
		logerror("%s for %zu: %d?", #thrd_call, our_id, _thrd_err); \
		break; \
	} \
} while (0)
#endif

typedef struct basic_thread_pool_todo {
	struct basic_thread_pool_todo *next;
	thrd_start_t func;
	void *arg;
} basic_thread_pool_todo_s;

typedef struct basic_thread_pool_loop_context {
	size_t id;
	basic_thread_pool_s *pool;
} basic_thread_pool_loop_context_s;

struct basic_thread_pool {
	mtx_t *mutex;
	cnd_t *todo;
	cnd_t *done;
	atomic_bool stop;
	thrd_t *threads;
	basic_thread_pool_loop_context_s *thread_contexts;
	size_t threads_len;
	size_t num_working;
	basic_thread_pool_todo_s *first;
	basic_thread_pool_todo_s *last;
};
/* typedef struct basic_thread_pool basic_thread_pool_s; */

static int basic_thread_pool_loop(void *arg)
{
	basic_thread_pool_loop_context_s *ctx = arg;
	size_t id = ctx->id;
	basic_thread_pool_s *pool = ctx->pool;

	while (true) {
		Tc(mtx_lock(pool->mutex), id);
		while (!pool->stop && pool->first == NULL) {
			Tc(cnd_wait(pool->todo, pool->mutex), id);
		}
		if (pool->stop) {
			Tc(cnd_broadcast(pool->done), id);
			Tc(mtx_unlock(pool->mutex), id);
			break;
		}

		if (pool->first != NULL) {
			basic_thread_pool_todo_s *elem = pool->first;
			pool->first = elem->next;
			++(pool->num_working);
			Tc(mtx_unlock(pool->mutex), id);

			if (elem != NULL) {
				void *arg = elem->arg;
				thrd_start_t func = elem->func;
				free(elem);
				func(arg);
			}

			Tc(mtx_lock(pool->mutex), id);
			--(pool->num_working);
			Tc(cnd_broadcast(pool->done), id);
		}
		Tc(mtx_unlock(pool->mutex), id);
		thrd_yield();
	}
	return 0;
}

basic_thread_pool_s *basic_thread_pool_new(size_t num_threads)
{
	num_threads = num_threads ? num_threads : 1;

	size_t size = sizeof(basic_thread_pool_s);
	basic_thread_pool_s *pool = malloc(size);
	malloc_or_log(&pool, sizeof(basic_thread_pool_s));

	pool->first = NULL;
	pool->last = NULL;
	pool->num_working = 0;
	pool->stop = false;

	size_t id = 0;
	int err = 0;

	malloc_or_log(&pool->mutex, sizeof(mtx_t));
	Tc((err = mtx_init(pool->mutex, mtx_plain)), id);
	if (err) {
		logerror("could not mtx_init(%s, %d)\n", "pool->mutex",
			 mtx_plain);
	}

	malloc_or_log(&pool->todo, sizeof(cnd_t));
	Tc((err = cnd_init(pool->todo)), id);
	if (err) {
		logerror("could not cnd_init(%s)\n", "pool->todo");
	}

	malloc_or_log(&pool->done, sizeof(cnd_t));
	Tc((err = cnd_init(pool->done)), id);
	if (err) {
		logerror("could not cnd_init(%s)\n", "pool->done");
	}

	size = sizeof(thrd_t) * num_threads;
	malloc_or_log(&pool->threads, size);

	size = sizeof(basic_thread_pool_loop_context_s) * num_threads;
	malloc_or_log(&pool->thread_contexts, size);

	pool->threads_len = num_threads;
	for (size_t i = 0; i < pool->threads_len; ++i) {
		id = 1 + i;
		pool->thread_contexts[i].pool = pool;
		pool->thread_contexts[i].id = id;

		thrd_t *thread = pool->threads + i;
		thrd_start_t func = basic_thread_pool_loop;
		void *arg = &(pool->thread_contexts[i]);
		Tc((err = thrd_create(thread, func, arg)), id);
		if (err) {
			logerror("could not (%s)\n", "pool->done");
		}
#ifndef DEBUG
		Tc(thrd_detach(*thread), id);
#endif
	}

	return pool;
}

int basic_thread_pool_add(basic_thread_pool_s *pool, thrd_start_t func,
			  void *arg)
{
	size_t id = 0;
	assert(pool);
	assert(func);

	basic_thread_pool_todo_s *elem;
	malloc_or_log(&elem, sizeof(basic_thread_pool_todo_s));
	if (!elem) {
		return 1;
	}
	elem->func = func;
	elem->arg = arg;
	elem->next = NULL;

	int err1;
	Tc((err1 = mtx_lock(pool->mutex)), id);
	if (pool->first == NULL) {
		pool->first = elem;
		pool->last = elem;
	} else {
		pool->last->next = elem;
		pool->last = elem;
	}
	int err2;
	Tc((err2 = cnd_broadcast(pool->todo)), id);
	int err3;
	Tc((err3 = mtx_unlock(pool->mutex)), id);

	return (err1 || err2 || err3) ? 1 : 0;
}

int basic_thread_pool_wait(basic_thread_pool_s *pool)
{
	size_t id = 0;
	assert(pool);
	int err1 = 0;
	int err2 = 0;
	Tc((err1 = mtx_lock(pool->mutex)), id);
	while ((pool->num_working > 0) || (pool->first != NULL)) {
		int err_tmp;
		Tc((err_tmp = cnd_wait(pool->done, pool->mutex)), id);
		if (err_tmp) {
			err2 = 1;
		}
	}
	int err3 = 0;
	Tc((err3 = mtx_unlock(pool->mutex)), id);
	return (err1 || err2 || err3) ? 1 : 0;
}

size_t basic_thread_pool_size(basic_thread_pool_s *pool)
{
	assert(pool);
	return pool->threads_len;
}

void basic_thread_pool_stop_and_free(basic_thread_pool_s *pool)
{
	size_t id = 0;
	assert(pool);

	Tc(mtx_lock(pool->mutex), id);

	pool->stop = true;

	while (pool->first != NULL) {
		basic_thread_pool_todo_s *elem = pool->first;
		pool->first = elem->next;
		free(elem);
	}

	Tc(cnd_broadcast(pool->todo), id);
	Tc(mtx_unlock(pool->mutex), id);

	Tc(mtx_lock(pool->mutex), id);
	while (pool->num_working > 0) {
		Tc(cnd_broadcast(pool->todo), id);
		Tc(cnd_wait(pool->done, pool->mutex), id);
	}
	Tc(cnd_broadcast(pool->todo), id);
	Tc(mtx_unlock(pool->mutex), id);
	thrd_yield();

	cnd_destroy(pool->todo);
	free(pool->todo);
	pool->todo = NULL;

	cnd_destroy(pool->done);
	free(pool->done);
	pool->done = NULL;

	mtx_destroy(pool->mutex);
	free(pool->mutex);
	pool->mutex = NULL;

#ifdef DEBUG
	for (size_t i = 0; i < pool->threads_len; ++i) {
		int result = 0;
		thrd_t thread = pool->threads[i];
		Tc(thrd_join(thread, &result), id);
		if (result) {
			fprintf(stderr, "thread[%zu] returned %d\n", i, result);
		}
	}
#endif

	free(pool->threads);
	free(pool->thread_contexts);
	free(pool);
}
