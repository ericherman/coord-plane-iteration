/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* basic-thread-pool.c: a basic thread pool */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include "basic-thread-pool.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "logerr-die.h"
#include "alloc-or-die.h"

#ifndef Make_valgrind_happy
#define Make_valgrind_happy 0
#endif

/* Thread command, check return code, and whine if not success */
static int tc(const char *str_thrd_call, int thrd_err, size_t our_id)
{
	static_assert(thrd_success == 0);

	switch (thrd_err) {
	case thrd_success:
		break;
	case thrd_nomem:
		errorf("%s for %zu: thrd_nomem", str_thrd_call, our_id);
		break;
	case thrd_error:
		errorf("%s for %zu: thrd_error", str_thrd_call, our_id);
		break;
	case thrd_timedout:
		errorf("%s for %zu: thrd_timedout", str_thrd_call, our_id);
		break;
	case thrd_busy:
		errorf("%s for %zu: thrd_busy", str_thrd_call, our_id);
		break;
	default:
		errorf("%s for %zu: %d?", str_thrd_call, our_id, thrd_err);
		break;
	}
	return thrd_err;
}

#define Tc(thrd_call, our_id) \
	tc(#thrd_call, thrd_call, our_id)

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
	bool stop;
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
	}
	return 0;
}

basic_thread_pool_s *basic_thread_pool_new(size_t num_threads)
{
	basic_thread_pool_s *pool = NULL;
	num_threads = num_threads ? num_threads : 1;

	pool = calloc_or_die("pool", 1, sizeof(basic_thread_pool_s));
	pool->mutex = calloc_or_die("pool->mutex", 1, sizeof(mtx_t));
	pool->todo = calloc_or_die("pool->todo", 1, sizeof(cnd_t));
	pool->done = calloc_or_die("pool->done", 1, sizeof(cnd_t));
	pool->threads =
	    calloc_or_die("pool->threads", num_threads, sizeof(thrd_t));
	pool->thread_contexts =
	    calloc_or_die("pool->thread_contexts", num_threads,
			  sizeof(basic_thread_pool_loop_context_s));

	size_t id = 0;

	if (Tc(mtx_init(pool->mutex, mtx_plain), id)) {
		die("could not mtx_init(%s, %d)\n", "pool->mutex", mtx_plain);
	}

	if (Tc(cnd_init(pool->todo), id)) {
		die("could not cnd_init(%s)\n", "pool->todo");
	}

	if (Tc(cnd_init(pool->done), id)) {
		die("could not cnd_init(%s)\n", "pool->done");
	}

	pool->threads_len = num_threads;
	for (size_t i = 0; i < pool->threads_len; ++i) {
		id = 1 + i;
		pool->thread_contexts[i].pool = pool;
		pool->thread_contexts[i].id = id;

		thrd_t *thread = pool->threads + i;
		thrd_start_t func = basic_thread_pool_loop;
		void *arg = &(pool->thread_contexts[i]);
		if (Tc(thrd_create(thread, func, arg), id)) {
			die("could not thrd_create (id: %zu)", id);
		}
#if !Make_valgrind_happy
		else {
			Tc(thrd_detach(*thread), id);
		}
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
	elem = calloc_or_log("elem", 1, sizeof(basic_thread_pool_todo_s));
	if (!elem) {
		return 1;
	}
	elem->func = func;
	elem->arg = arg;

	if (Tc(mtx_lock(pool->mutex), id)) {
		die("could not mtx_lock(pool->mutex) (%p) (id: %zu)",
		    pool->mutex, id);
	}

	if (pool->stop) {
		Tc(mtx_unlock(pool->mutex), id);
		return 1;
	}

	if (pool->first == NULL) {
		pool->first = elem;
		pool->last = elem;
	} else {
		pool->last->next = elem;
		pool->last = elem;
	}
	int err = Tc(cnd_broadcast(pool->todo), id);
	if (Tc(mtx_unlock(pool->mutex), id)) {
		die("could not mtx_unlock(pool->mutex) (%p) (id: %zu)",
		    pool->mutex, id);
	}

	return err;
}

int basic_thread_pool_wait(basic_thread_pool_s *pool)
{
	size_t id = 0;
	assert(pool);
	if (Tc(mtx_lock(pool->mutex), id)) {
		die("failed to mtx_lock(%p), %zu", pool->mutex, id);
	}
	int err = 0;
	while ((pool->num_working > 0) || (pool->first != NULL)) {
		if (Tc(cnd_wait(pool->done, pool->mutex), id)) {
			err = 1;
		}
	}
	if (Tc(mtx_unlock(pool->mutex), id)) {
		die("failed to mtx_unlock(%p), %zu", pool->mutex, id);
	}
	return err;
}

size_t basic_thread_pool_size(basic_thread_pool_s *pool)
{
	assert(pool);
	return pool->threads_len;
}

void basic_thread_pool_stop_and_free(basic_thread_pool_s **pool_ref)
{
	basic_thread_pool_s *pool = *pool_ref;
	size_t id = 0;
	if (!pool) {
		return;
	}

	Tc(mtx_lock(pool->mutex), id);

	pool->stop = true;

	while (pool->first != NULL) {
		basic_thread_pool_todo_s *elem = pool->first;
		pool->first = elem->next;
		free(elem);
	}

	Tc(cnd_broadcast(pool->todo), id);

	while (pool->num_working > 0) {
		Tc(cnd_broadcast(pool->todo), id);
		Tc(cnd_wait(pool->done, pool->mutex), id);
	}

	Tc(cnd_broadcast(pool->todo), id);
	Tc(mtx_unlock(pool->mutex), id);

	for (size_t i = 0; i < pool->threads_len; ++i) {
		int result = 0;
		thrd_t thread = pool->threads[i];
		if (thread) {
			/* Note: if Make_valgrind_happy, we're joining a
			   thread which was not thrd_detach()-ed,
			   this is undefined behavior, but works
			   on amd64 linux with glibc */
			Tc(thrd_join(thread, &result), id);
		}
		if (result) {
			/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01 */
			fflush(stdout);
			fprintf(stderr, "\nthread[%zu] returned %d\n", i,
				result);
		}
	}

	free(pool->threads);
	pool->threads = NULL;

	free(pool->thread_contexts);
	pool->thread_contexts = NULL;

	cnd_destroy(pool->todo);
	free(pool->todo);
	pool->todo = NULL;

	cnd_destroy(pool->done);
	free(pool->done);
	pool->done = NULL;

	mtx_destroy(pool->mutex);
	free(pool->mutex);
	pool->mutex = NULL;

	free(pool);
	*pool_ref = NULL;
}
