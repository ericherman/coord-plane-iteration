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

/* Thread command, check return code, and whine if not success */
static int tc(const char *str_thrd_call, int thrd_err, size_t our_id)
{
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
	basic_thread_pool_s *pool = NULL;
	pool = malloc_or_log("pool", sizeof(basic_thread_pool_s));
	if (!pool) {
		return NULL;
	}
	memset(pool, 0x00, sizeof(basic_thread_pool_s));

	pool->first = NULL;
	pool->last = NULL;
	pool->num_working = 0;
	pool->stop = false;

	pool->mutex = malloc_or_log("pool->mutex", sizeof(mtx_t));
	pool->todo = malloc_or_log("pool->todo", sizeof(cnd_t));
	pool->done = malloc_or_log("pool->done", sizeof(cnd_t));

	size = sizeof(thrd_t) * num_threads;
	pool->threads = malloc_or_log("pool->threads", size);

	size = sizeof(basic_thread_pool_loop_context_s) * num_threads;
	pool->thread_contexts = malloc_or_log("pool->thread_contexts", size);

	int err = !(pool->mutex && pool->todo && pool->done && pool->threads
		    && pool->thread_contexts);
	if (err) {
		errorf("allocation failed,"
		       " mutex: %p todo:%p done: %p"
		       " threads %p thread_contexts: %p", pool->mutex,
		       pool->todo, pool->done, pool->threads,
		       pool->thread_contexts);
		goto basic_thread_pool_new_end;
	}

	size_t id = 0;

	err = Tc(mtx_init(pool->mutex, mtx_plain), id);
	if (err) {
		errorf("could not mtx_init(%s, %d)\n", "pool->mutex",
		       mtx_plain);
		goto basic_thread_pool_new_end;
	}

	err = Tc(cnd_init(pool->todo), id);
	if (err) {
		errorf("could not cnd_init(%s)\n", "pool->todo");
		goto basic_thread_pool_new_end;
	}

	err = Tc(cnd_init(pool->done), id);
	if (err) {
		errorf("could not cnd_init(%s)\n", "pool->done");
		goto basic_thread_pool_new_end;
	}

	pool->threads_len = num_threads;
	for (size_t i = 0; i < pool->threads_len; ++i) {
		id = 1 + i;
		pool->thread_contexts[i].pool = pool;
		pool->thread_contexts[i].id = id;

		thrd_t *thread = pool->threads + i;
		thrd_start_t func = basic_thread_pool_loop;
		void *arg = &(pool->thread_contexts[i]);
		err = Tc(thrd_create(thread, func, arg), id);
		if (err) {
			errorf("could not (%s)\n", "thrd_create");
			goto basic_thread_pool_new_end;
		}
#ifndef DEBUG
		else {
			Tc(thrd_detach(*thread), id);
		}
#endif
	}

basic_thread_pool_new_end:
	if (err) {
		basic_thread_pool_stop_and_free(&pool);
		return NULL;
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
	elem = malloc_or_log("elem", sizeof(basic_thread_pool_todo_s));
	if (!elem) {
		return 1;
	}
	elem->func = func;
	elem->arg = arg;
	elem->next = NULL;

	int err1 = Tc(mtx_lock(pool->mutex), id);
	if (pool->first == NULL) {
		pool->first = elem;
		pool->last = elem;
	} else {
		pool->last->next = elem;
		pool->last = elem;
	}
	int err2 = Tc(cnd_broadcast(pool->todo), id);
	int err3 = Tc(mtx_unlock(pool->mutex), id);

	return (err1 || err2 || err3) ? 1 : 0;
}

int basic_thread_pool_wait(basic_thread_pool_s *pool)
{
	size_t id = 0;
	assert(pool);
	int err1 = Tc(mtx_lock(pool->mutex), id);
	int err2 = 0;
	while ((pool->num_working > 0) || (pool->first != NULL)) {
		int err_tmp = Tc(cnd_wait(pool->done, pool->mutex), id);
		if (err_tmp) {
			err2 = 1;
		}
	}
	int err3 = Tc(mtx_unlock(pool->mutex), id);
	return (err1 || err2 || err3) ? 1 : 0;
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

	if (pool->mutex) {
		Tc(mtx_lock(pool->mutex), id);
	}

	pool->stop = true;

	while (pool->first != NULL) {
		basic_thread_pool_todo_s *elem = pool->first;
		pool->first = elem->next;
		free(elem);
	}

	if (pool->todo) {
		Tc(cnd_broadcast(pool->todo), id);
	}
	if (pool->mutex) {
		Tc(mtx_unlock(pool->mutex), id);
	}

	if (pool->mutex) {
		Tc(mtx_lock(pool->mutex), id);
	}
	while (pool->num_working > 0) {
		if (pool->todo) {
			Tc(cnd_broadcast(pool->todo), id);
		}
		if (pool->done && pool->mutex) {
			Tc(cnd_wait(pool->done, pool->mutex), id);
		}
	}
	if (pool->todo) {
		Tc(cnd_broadcast(pool->todo), id);
	}
	if (pool->mutex) {
		Tc(mtx_unlock(pool->mutex), id);
	}
	thrd_yield();

	if (pool->todo) {
		cnd_destroy(pool->todo);
		free(pool->todo);
		pool->todo = NULL;
	}

	if (pool->done) {
		cnd_destroy(pool->done);
		free(pool->done);
		pool->done = NULL;
	}

#ifdef DEBUG
	for (size_t i = 0; i < pool->threads_len; ++i) {
		int result = 0;
		thrd_t thread = pool->threads[i];
		if (thread) {
			Tc(thrd_join(thread, &result), id);
		}
		if (result) {
			/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01 */
			fflush(stdout);
			fprintf(stderr, "thread[%zu] returned %d\n", i, result);
		}
	}
#endif

	if (pool->threads) {
		free(pool->threads);
		pool->threads = NULL;
	}
	if (pool->thread_contexts) {
		free(pool->thread_contexts);
		pool->thread_contexts = NULL;
	}
	if (pool->mutex) {
		mtx_destroy(pool->mutex);
		free(pool->mutex);
		pool->mutex = NULL;
	}
	free(pool);
	*pool_ref = NULL;
}
