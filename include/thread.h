
#ifndef THREAD_H
#define THREAD_H

#include <parser.h>
#include <rbtree.h>
#include <propes.h>

/** this structure are protected */
typedef struct locker_s locker_t;
/** this structure are protected */
typedef struct thread_s thread_t;
/** this structure are protected */
typedef struct method_s method_t;
/** this structure are protected */
typedef struct module_s module_t;

typedef enum thread_lock_e thread_lock_t;
typedef enum thread_state_e thread_state_t;
typedef enum thread_method_state_e thread_method_state_t;

enum thread_state_e {

	THREAD_STATE_STOPPED =  0,
	THREAD_STATE_STARTED =  1,
	THREAD_STATE_INVALID = -1,
};

enum thread_method_state_e {

	THREAD_METHOD_ERROR = -1,
	THREAD_METHOD_OK    =  0,
};

enum thread_lock_e {

	THREAD_LOCK_READ    = 1,
	THREAD_UNLOCK_READ  = -1,
	THREAD_LOCK_WRITE   = 2,
	THREAD_UNLOCK_WRITE = -2,
};

struct method_s {

	char* name;
	int (*run)(thread_t* thread, json_node_t* request, json_node_t* answer);
	char* description;
	char* jsont;
};

struct module_s {

	char* name;
	char* description;

	method_t* methods;
	property_t* props;

	void (*routine_f)(thread_t*);

	void* (*on_create_f)(thread_t*);
	void (*on_destroy_f)(thread_t*);
	void (*on_start_f)(thread_t*);
	void (*on_stop_f)(thread_t*);

	void (*on_save_f)(thread_t*, const char*);
	void (*on_init_f)();
};

/** create locker_t struct */
locker_t* locker_create();

/** destroy locker_t */
void locker_destroy(void* data);

/** locker set mode */
int locker_set(locker_t* locker, thread_lock_t mode);

/** create thread_t struct */
thread_t* thread_create(module_t* module, const char* name);

/** destroy thread_t */
void thread_destroy(void* data);

/** set thread state from other thread */
void thread_state_set(thread_t* thread, thread_state_t state);

/** get thread state */
thread_state_t thread_state(thread_t* thread);

/** update thread from the runned thread */
void thread_update(thread_t* thread, thread_state_t state);

/** get thread state name */
const char* thread_state_str(thread_state_t state);

/** get thread name */
char* thread_name(thread_t* thread);

/** get thread module */
module_t* thread_module(thread_t* thread);

/** get thread method */
method_t* thread_method(thread_t* thread, const char* name);

/** wait running thread */
thread_state_t thread_run_wait(thread_t* thread);

/** get thread propes */
propes_t* thread_propes(thread_t* thread);

/** get thread data */
void* thread_data(thread_t* thread);

/** get thread locker */
locker_t* thread_locker(thread_t* thread);

/** get thread info */
int thread_info(thread_t* thread, json_node_t* info);

#endif // THREAD_H
