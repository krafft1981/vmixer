
#ifndef LOADER_H
#define LOADER_H

#include <parser.h>
#include <thread.h>

/** this structure are protected */
typedef struct loader_s loader_t;

/** create loader */
loader_t* loader_create(const char* file);

/** destroy loader */
void loader_destroy(void* data);

/** get loader module */
module_t* loader_module(loader_t* loader);

/** get loader name */
const char* loader_name(loader_t* loader);

/** get loader file */
const char* loader_file(loader_t* loader);

/** answer loader info */
int loader_info(loader_t* loader, json_node_t* info);

/** set to loader */
int set_to_loader(loader_t* loader, thread_t* thread);

/** get from loader */
thread_t* get_from_loader(loader_t* loader, const char* name);

/** get loader locker */
locker_t* loader_locker(loader_t* loader);

#endif // LOADER_H
