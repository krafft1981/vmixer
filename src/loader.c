
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "propes.h"
#include "rbtree.h"
#include "logger.h"
#include "loader.h"

#define EXPORT "export"

struct loader_s {

	const char *file;
	void *handle;
	module_t* module;
	locker_t* locker;
	rbtree_t* pool;
};

loader_t* loader_create(const char* file) {

	if (!file)
		return NULL;

	loader_t* loader = malloc(sizeof(*loader));
	if (loader) {
		loader->file = file;
		loader->handle = dlopen(file, RTLD_NOW);
		if (!loader->handle) {
			ERROR("%s", dlerror());
			free(loader);
			return NULL;
		}

		loader->module = dlsym(loader->handle, EXPORT);
		if (!loader->module) {
			ERROR("%s", dlerror());
			dlclose(loader->handle);
			free(loader);
			return NULL;
		}

		loader->pool = rbtree_create(NULL, thread_destroy);
		loader->locker = locker_create();

		if (loader->module->on_init_f) {
			DEBUG("module: \"%s\" on_init() started", loader->module->name, loader->module->on_init_f);
			loader->module->on_init_f();
			DEBUG("module: \"%s\" on_init() finished.", loader->module->name, loader->module->on_init_f);
		}

		INFO("module: '%s' loaded file: '%s'.", loader->module->name, file);
	}

	return loader;
}

void loader_destroy(void* data) {

	if (data) {
		loader_t* loader = data;
		INFO("module: '%s' unloaded. file: '%s'", loader->module->name, loader->file);
		rbtree_destroy(loader->pool);
		locker_destroy(loader->locker);
		dlclose(loader->handle);
		free(data);
	}
}

module_t* loader_module(loader_t* loader) {

	if (!loader)
		return NULL;

	return loader->module;
}

const char* loader_name(loader_t* loader) {

	if (!loader)
		return NULL;

	return loader->module->name;
}

int set_to_loader(loader_t* loader, thread_t* thread) {

	if (!loader || !thread)
		return -1;

	locker_set(loader->locker, THREAD_LOCK_WRITE);
	int res = set_to_rbtree(loader->pool, thread_name(thread), thread);
	locker_set(loader->locker, THREAD_UNLOCK_WRITE);
	return res;
}

thread_t* get_from_loader(loader_t* loader, const char* name) {

	if (!loader || !name)
		return NULL;

	return get_from_rbtree(loader->pool, name);
}

locker_t* loader_locker(loader_t* loader) {

	if (!loader)
		return NULL;

	return loader->locker;
}

const char* loader_file(loader_t* loader) {

	if (loader)
		return loader->file;

	return NULL;
}

int loader_info(loader_t* loader, json_node_t* info) {

	if (!loader || !info)
		return -1;

	json_node_object_add(info, "description", json_node_string(loader->module->description));
	json_node_object_add(info, "method", json_node_object(NULL));

	if (loader->module->methods) {
		int id = 0;
		while (loader->module->methods[id].name) {
			json_node_t* method = json_node_object(NULL);
			json_node_object_add(method, "description", json_node_string(loader->module->methods[id].description));
			json_node_object_add(method, "jsont", json_node_string(loader->module->methods[id].jsont));
			json_node_object_add(json_node_object_node(info, "method", JSON_NODE_TYPE_OBJECT), loader->module->methods[id].name, method);
			id ++;
		}
	}

	json_node_object_add(info, "property", json_node_object(NULL));

	if (loader->module->props) {
		int id = 0;
		while (loader->module->props[id].name) {
			json_node_t* property = json_node_object(NULL);
			json_node_object_add(property, "description", json_node_string(loader->module->props[id].description));
			json_node_object_add(property, "defval", json_node_string(loader->module->props[id].defval));
			json_node_object_add(json_node_object_node(info, "property", JSON_NODE_TYPE_OBJECT), loader->module->props[id].name, property);
			id ++;
		}
	}

	json_node_object_add(info, "pool", json_node_object(NULL));
	rbtree_iterator_t* it = rbtree_iterator_create(loader->pool);

	const char* key;
	void* data;

	locker_set(loader->locker, THREAD_LOCK_READ);
	while (rbtree_iterate(it, &key, &data)) {
		json_node_t* thread = json_node_object(NULL);
		thread_info(data, thread);
		json_node_object_add(json_node_object_node(info, "pool", JSON_NODE_TYPE_OBJECT), thread_name(data), thread);
	}
	locker_set(loader->locker, THREAD_UNLOCK_READ);
	rbtree_iterator_destroy(it);

	return 0;
}
