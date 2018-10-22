
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "logger.h"
#include "thread.h"

struct locker_s {

	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int readed;
	int writed;
};

struct thread_s {

	char name[128];

	pthread_t td;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	thread_state_t state;
	locker_t* locker;
	module_t* module;
	propes_t* props;

	void* data;
};

static void routine(thread_t* thread) {

	DEBUG("\'%s\':\'%s\' [%lu] routine(%p) started", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->routine_f);
	thread->module->routine_f(thread);
	DEBUG("\'%s\':\'%s\' [%lu] routine(%p) finished.", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->routine_f);

	if (thread_state(thread) != THREAD_STATE_INVALID)
		thread_update(thread, THREAD_STATE_STOPPED);
}

thread_t* thread_create(module_t* module, const char* name) {

	if (!module || !name)
		return NULL;

	thread_t* thread = calloc(1, sizeof(*thread));
	if (!thread)
		return thread;

	strncpy(thread->name, name, sizeof(thread->name));

	thread->props = propes_create(module->props);
	thread->state = THREAD_STATE_STOPPED;
	thread->locker = locker_create();
	thread->module = module;

	DEBUG("\'%s\':\'%s\' create request", thread_module(thread)->name, thread_name(thread));

	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);

	if (thread->module->on_create_f) {
		DEBUG("\'%s\':\'%s\' on_create(%p) started", thread_module(thread)->name, thread_name(thread), thread->module->on_create_f);
		thread->data = thread->module->on_create_f(thread);
		DEBUG("\'%s\':\'%s\' on_create(%p) finished.", thread_module(thread)->name, thread_name(thread), thread->module->on_create_f);
	}

	INFO("\'%s\':\'%s\' create complete success.", thread_module(thread)->name, thread_name(thread));
	return thread;
}

void thread_destroy(void* data) {

	if (!data)
		return;

	thread_t* thread = data;

	DEBUG("\'%s\':\'%s\' destroy request", thread_module(thread)->name, thread_name(thread));

	if (thread_state(thread) != THREAD_STATE_STOPPED) {
		thread_state_set(thread, THREAD_STATE_STOPPED);
		DEBUG("\'%s\':\'%s\' return to destroy", thread_module(thread)->name, thread_name(thread));
	}

	if (thread->td) {
		pthread_join(thread->td, NULL);
		thread->td = 0;
	}

	if (thread->module->on_destroy_f) {
		DEBUG("\'%s\':\'%s\' on_destroy(%p) started", thread_module(thread)->name, thread_name(thread), thread->module->on_destroy_f);
		thread->module->on_destroy_f(thread);
		DEBUG("\'%s\':\'%s\' on_destroy(%p) finished.", thread_module(thread)->name, thread_name(thread), thread->module->on_destroy_f);
	}

	propes_destroy(thread->props);
	locker_destroy(thread->locker);

	pthread_cond_destroy(&thread->cond);
	pthread_mutex_destroy(&thread->mutex);

	INFO("\'%s\':\'%s\' destroy complete success.", thread_module(thread)->name, thread_name(thread));
}

const char* thread_state_str(thread_state_t state) {

	switch (state) {
		case THREAD_STATE_STARTED: return "started";
		case THREAD_STATE_STOPPED: return "stopped";
		case THREAD_STATE_INVALID: return "invalid";
		default:
			return "Shit happens";
	}
}

thread_state_t thread_state(thread_t *thread) {

	pthread_mutex_lock(&thread->mutex);
	thread_state_t state = thread->state;
	DEBUG("\'%s\':\'%s\' has state \"%s\"", thread_module(thread)->name, thread_name(thread), thread_state_str(state));
	pthread_mutex_unlock(&thread->mutex);

	return state;
}

void thread_update(thread_t* thread, thread_state_t state) {

	pthread_mutex_lock(&thread->mutex);
	thread->state = state;
	pthread_cond_broadcast(&thread->cond);
	DEBUG("\'%s\':\'%s\' [%lu] update state set to \"%s\"", thread_module(thread)->name, thread_name(thread), pthread_self(), thread_state_str(thread->state));
	pthread_mutex_unlock(&thread->mutex);
}

void thread_state_set(thread_t* thread, thread_state_t state) {

	if (!thread)
		return;

	DEBUG("\'%s\':\'%s\' try set state \"%s\"", thread_module(thread)->name, thread_name(thread), thread_state_str(state));

	locker_set(thread->locker, THREAD_LOCK_WRITE);
	if (thread_state(thread) == state)
		WARN("\'%s\':\'%s\' already state: \"%s\"", thread_module(thread)->name, thread_name(thread), thread_state_str(thread->state));

	else {
		switch (state) {
			case THREAD_STATE_STARTED: {
				if (thread->td) {
					pthread_join(thread->td, NULL);
					thread->td = 0;
				}

				if (thread->module->routine_f) {
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_mutex_lock(&thread->mutex);

					if (pthread_create(&thread->td, &attr, (void* (*)(void *)) routine, thread)) {
						ERROR("pthread_create: %s", strerror(errno));
						thread->state = THREAD_STATE_INVALID;
						thread->td = 0;
					}

					else {
						DEBUG("\'%s\':\'%s\' [%lu] wait for thread update call", thread_module(thread)->name, thread_name(thread), pthread_self());
						pthread_cond_wait(&thread->cond, &thread->mutex);
						DEBUG("\'%s\':\'%s\' [%lu] thread update called", thread_module(thread)->name, thread_name(thread), pthread_self());
					}

					pthread_mutex_unlock(&thread->mutex);
					pthread_attr_destroy(&attr);
				}

				else {
					pthread_mutex_lock(&thread->mutex);
					thread->state = THREAD_STATE_STARTED;
					pthread_mutex_unlock(&thread->mutex);
				}

				pthread_mutex_lock(&thread->mutex);
				thread_state_t change = thread->state;
				pthread_mutex_unlock(&thread->mutex);

				if (change == THREAD_STATE_STARTED) {
					if (thread->module->on_start_f) {
						DEBUG("\'%s\':\'%s\' [%lu] on_start(%p) started", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->on_start_f);
						thread->module->on_start_f(thread);
						DEBUG("\'%s\':\'%s\' [%lu] on_start(%p) finished.", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->on_start_f);
					}
				}

				break;
			}

			case THREAD_STATE_STOPPED: {
				pthread_mutex_lock(&thread->mutex);
				thread->state = THREAD_STATE_STOPPED;
				pthread_mutex_unlock(&thread->mutex);
				if (thread->module->on_stop_f) {
					DEBUG("\'%s\':\'%s\' [%lu] on_stop(%p) started", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->on_stop_f);
					thread->module->on_stop_f(thread);
					DEBUG("\'%s\':\'%s\' [%lu] on_stop(%p) finished.", thread_module(thread)->name, thread_name(thread), pthread_self(), thread->module->on_stop_f);
				}

				if (thread->td) {
					pthread_join(thread->td, NULL);
					thread->td = 0;
				}

				break;
			}

			case THREAD_STATE_INVALID:
			default: {
				pthread_mutex_lock(&thread->mutex);
				thread->state = THREAD_STATE_INVALID;
				pthread_mutex_unlock(&thread->mutex);

				if (thread->td) {
					pthread_join(thread->td, NULL);
					thread->td = 0;
				}

				break;
			}
		}

		INFO("\'%s\':\'%s\' set state \"%s\" complete.", thread_module(thread)->name, thread_name(thread), thread_state_str(state));
	}

	locker_set(thread->locker, THREAD_UNLOCK_WRITE);
}

thread_state_t thread_run_wait(thread_t* thread) {

	if (!thread)
		return THREAD_STATE_INVALID;

	INFO("\'%s\':\'%s\' run wait [%lu] from [%lu] started", thread_module(thread)->name, thread_name(thread), thread->td, pthread_self());
	thread_state_t state = thread_state(thread);

	while (state == THREAD_STATE_STARTED) {
		pthread_mutex_lock(&thread->mutex);
		pthread_cond_wait(&thread->cond, &thread->mutex);
		state = thread->state;
		pthread_mutex_unlock(&thread->mutex);
	}

	INFO("\'%s\':\'%s\' run wait [%lu] from [%lu] with \"%s\" finished.", thread_module(thread)->name, thread_name(thread), thread->td, pthread_self(), thread_state_str(state));
	return state;
}

char* thread_name(thread_t* thread) {

	if (thread)
		return thread->name;

	return NULL;
}

module_t* thread_module(thread_t* thread) {

	if (thread)
		return thread->module;

	return NULL;
}

method_t* thread_method(thread_t* thread, const char* name) {

	if (!thread || !name)
		return NULL;

	if (thread->module->methods) {
		int id = 0;
		while (thread->module->methods[id].name) {
			if (!strcmp(thread->module->methods[id].name, name))
				return &thread->module->methods[id];

			id ++;
		}
	}

	return NULL;
}

propes_t* thread_propes(thread_t* thread) {

	if (!thread)
		return NULL;

	return thread->props;
}

void* thread_data(thread_t* thread) {

	if (!thread)
		return NULL;

	return thread->data;
}

locker_t* thread_locker(thread_t* thread) {

	if (!thread)
		return NULL;

	return thread->locker;
}

locker_t* locker_create() {

	locker_t* locker = calloc(1, sizeof(*locker));
	if (locker) {
		pthread_mutex_init(&locker->mutex, NULL);
		pthread_cond_init(&locker->cond, NULL);
		DEBUG("locker [%lu] created.", locker);
	}

	return locker;
}

void locker_destroy(void* data) {

	if (data) {
		locker_t* locker = data;
		pthread_cond_destroy(&locker->cond);
		pthread_mutex_destroy(&locker->mutex);
		DEBUG("locker [%lu] destroyed.", locker);
		free(data);
	}
}

int locker_set(locker_t* locker, thread_lock_t mode) {

	if (!locker)
		return -1;

	switch (mode) {

		case THREAD_LOCK_READ: {
			pthread_mutex_lock(&locker->mutex);
			DEBUG("locker [%lu] lock(read) start at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());

			while (locker->writed) {
				pthread_cond_wait(&locker->cond, &locker->mutex);
				DEBUG("locker [%lu] lock(read) signal at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			}

			locker->readed ++;
			DEBUG("locker [%lu] lock(read) stop at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			pthread_mutex_unlock(&locker->mutex);
			break;
		}

		case THREAD_UNLOCK_READ: {
			pthread_mutex_lock(&locker->mutex);
			DEBUG("locker [%lu] unlock(read) (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			locker->readed --;
			pthread_cond_broadcast(&locker->cond);
			pthread_mutex_unlock(&locker->mutex);
			break;
		}

		case THREAD_LOCK_WRITE: {
			pthread_mutex_lock(&locker->mutex);
			DEBUG("locker [%lu] lock(write) start at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());

			while (locker->writed || locker->readed) {
				pthread_cond_wait(&locker->cond, &locker->mutex);
				DEBUG("locker [%lu] lock(write) signal at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			}

			locker->writed ++;
			DEBUG("locker [%lu] lock(write) stop at (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			pthread_mutex_unlock(&locker->mutex);
			break;
		}

		case THREAD_UNLOCK_WRITE: {
			pthread_mutex_lock(&locker->mutex);
			DEBUG("locker [%lu] unlock(write) (r(%d) w(%d)) from [%lu]", locker, locker->readed, locker->writed, pthread_self());
			locker->writed --;
			pthread_cond_broadcast(&locker->cond);
			pthread_mutex_unlock(&locker->mutex);
			break;
		}

		default:
			return -1;
	}

	return 0;
}

int thread_info(thread_t* thread, json_node_t* info) {

	if (!thread || !info)
		return -1;

	locker_set(thread->locker, THREAD_LOCK_READ);
	json_node_object_add(info, "state", json_node_string(thread_state_str(thread_state(thread))));
	json_node_object_add(info, "property", json_node_object(NULL));
	if (thread->module->props) {
		int id = 0;
		while (thread->module->props[id].name) {
			json_node_object_add(json_node_object_node(info, "property", JSON_NODE_TYPE_OBJECT), thread->module->props[id].name, json_node_string(get_from_propes(thread->props, thread->module->props[id].name)));
			id ++;
		}
	}
	locker_set(thread->locker, THREAD_UNLOCK_READ);
	return 0;
}
