
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "logger.h"
#include "thread.h"
#include "vector.h"
#include "client.h"

typedef struct cluster_node_s cluster_node_t;

struct cluster_node_s {

	address_t* address;
	struct {
		int score;
	} stat;
};

struct cluster_s {

	vector_t* vector;
};

struct client_s {

	int sock;

	parser_t* parser;
	address_t* address;
	cluster_t* cluster;
};

int client_connect(address_t* address) {

	int sock;

#ifdef ENABLE_TCP
	if (!strcmp(address_get_proto(address), "tcp")) {
		struct sockaddr_in srv = { 0 };
		srv.sin_family = AF_INET;
		srv.sin_port = htons(address_get_port(address));
		inet_pton(AF_INET, address_get_host(address), &srv.sin_addr.s_addr);
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {}
		int optarg = 1;
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optarg, sizeof(optarg));
		if (connect(sock, (struct sockaddr *)&srv, sizeof(struct sockaddr))) {
			ERROR("connect to \"%s\": %s", address_get_url(address), strerror(errno));
			return -1;
		}
	}
#endif
#ifdef ENABLE_SCTP
	if (!strcmp(address_get_proto(address), "sctp")) {
		struct sockaddr_in srv = { 0 };
		srv.sin_family = AF_INET;
		srv.sin_port = htons(address_get_port(address));
		inet_pton(AF_INET, address_get_host(address), &srv.sin_addr.s_addr);
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1) {}
		int optarg = 1;
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optarg, sizeof(optarg));
		if (connect(sock, (struct sockaddr *)&srv, sizeof(struct sockaddr))) {
			ERROR("connect to \"%s\": %s", address_get_url(address), strerror(errno));
			return -1;
		}
	}
#endif
#ifdef ENABLE_UNIX
	if (!strcmp(address_get_proto(address), "unix")) {
		struct sockaddr_un srv = { 0 };
		srv.sun_family = AF_UNIX;
		snprintf(srv.sun_path, sizeof(srv.sun_path), "%s", address_get_host(address));
		if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {}
		int optarg = 1;
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optarg, sizeof(optarg));
		if (connect(sock, (struct sockaddr *)&srv, sizeof(struct sockaddr))) {
			ERROR("connect to \"%s\": %s", address_get_url(address), strerror(errno));
			return -1;
		}
	}
#endif
#ifdef ENABLE_UDP
	if (!strcmp(address_get_proto(address), "udp")) {
		struct sockaddr_un srv = { 0 };
		srv.sun_family = AF_UNIX;
		snprintf(srv.sun_path, sizeof(srv.sun_path), "%s", address_get_host(address));
		if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {}
		int optarg = 1;
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optarg, sizeof(optarg));
	}
#endif

	return sock;
}

client_t* client_cluster(cluster_t* cluster) {

	
}

client_t* client_create(const char* url) {

	client_t* client = calloc(1, sizeof(*client));
	if (client) {
		if (!(client->address = address_create(url))) {
			client_destroy(client);
			return NULL;
		}

		if (!(client->parser = parser_create())) {
			client_destroy(client);
			return NULL;
		}

		if ((client->sock = client_connect(client->address)) == -1) {
			client_destroy(client);
			return NULL;
		}
	}

	return client;
}

void client_destroy(void* data) {

	if (data) {
		client_t* client = data;
		if (client->sock > 0)
			close(client->sock);
		address_destroy(client->address);
		parser_destroy(client->parser);
		free(data);
	}
}

int client_request(client_t* client, const char* module, const char* thread, const char* method, json_node_t* args, json_node_t* *answer) {

	if (!client || !answer)
		return THREAD_METHOD_ERROR;

	json_node_t* request = json_node_object(NULL);
	json_node_object_add(request, "target", json_node_object(NULL));
	json_node_object_add(request, "args"  , args);
	json_node_object_add(json_node_object_node(request, "target", JSON_NODE_TYPE_OBJECT), "module", json_node_string(module));
	json_node_object_add(json_node_object_node(request, "target", JSON_NODE_TYPE_OBJECT), "thread", json_node_string(thread));
	json_node_object_add(json_node_object_node(request, "target", JSON_NODE_TYPE_OBJECT), "method", json_node_string(method));

	char buffer[IO_BUFFER_SIZE]; buffer[0] = '\0';
	int size = IO_BUFFER_SIZE;

	if (json_node_print(request, JSON_STYLE_MINIMAL, &size, buffer)) {
		json_node_destroy(request);
		return THREAD_METHOD_ERROR;
	}

	size = IO_BUFFER_SIZE - size;

	if (!strcmp(address_get_proto(client->address), "tcp")) {
#ifdef ENABLE_TCP
		if (write(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		if (write(client->sock, buffer, size) != size)
			return THREAD_METHOD_ERROR;

		if (read(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		int readed = 0;
		while (readed != size) {
			int msgsize = read(client->sock, &buffer[readed], size - readed);
			if (msgsize <= 0)
				break;
			readed += msgsize;
		}

		if (readed != size)
			return THREAD_METHOD_ERROR;

		*answer = parser_parse_buffer(client->parser, buffer, size);
#else
		*answer = json_node_object(NULL);
		json_node_object_add(*answer, "error", json_node_string("protocol not compiled"));
		return THREAD_METHOD_ERROR;
#endif
	}
	if (!strcmp(address_get_proto(client->address), "udp")) {
#ifdef ENABLE_UDP
		//send json_node via client_t*
		//recv json_node via client_t*
#else
		*answer = json_node_object(NULL);
		json_node_object_add(*answer, "error", json_node_string("protocol not compiled"));
		return THREAD_METHOD_ERROR;
#endif
	}
	if (!strcmp(address_get_proto(client->address), "local")) {
#ifdef ENABLE_LOCAL
		if (write(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		if (write(client->sock, buffer, size) != size)
			return THREAD_METHOD_ERROR;

		if (read(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		int readed = 0;
		while (readed != size) {
			int msgsize = read(client->sock, &buffer[readed], size - readed);
			if (msgsize <= 0)
				break;
			readed += msgsize;
		}

		if (readed != size)
			return THREAD_METHOD_ERROR;

		*answer = parser_parse_buffer(client->parser, buffer, size);
#else
		*answer = json_node_object(NULL);
		json_node_object_add(*answer, "error", json_node_string("protocol not compiled"));
		return THREAD_METHOD_ERROR;
#endif
	}
	if (!strcmp(address_get_proto(client->address), "sctp")) {
#ifdef ENABLE_SCTP
		if (write(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		if (write(client->sock, buffer, size) != size)
			return THREAD_METHOD_ERROR;

		if (read(client->sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			return THREAD_METHOD_ERROR;

		int readed = 0;
		while (readed != size) {
			int msgsize = read(client->sock, &buffer[readed], size - readed);
			if (msgsize <= 0)
				break;
			readed += msgsize;
		}

		if (readed != size)
			return THREAD_METHOD_ERROR;

		*answer = parser_parse_buffer(client->parser, buffer, size);
#else
		*answer = json_node_object(NULL);
		json_node_object_add(*answer, "error", json_node_string("protocol not compiled"));
		return THREAD_METHOD_ERROR;
#endif
	}

	return THREAD_METHOD_OK;
}

int client_file_request(client_t* client, const char* file, json_node_t* *answer) {

	if (!client || !file || !answer)
		return THREAD_METHOD_ERROR;

	json_node_t* request = parser_parse_file(client->parser, file);
	if (!request)
		return THREAD_METHOD_ERROR;

	return client_request(client,
		json_node_string_value(json_node_object_node(request, "module", JSON_NODE_TYPE_STRING)),
		json_node_string_value(json_node_object_node(request, "thread", JSON_NODE_TYPE_STRING)),
		json_node_string_value(json_node_object_node(request, "method", JSON_NODE_TYPE_STRING)),
		json_node_object_node(request, "args", JSON_NODE_TYPE_OBJECT),
		answer
	);
}

cluster_t* cluster_create() {

	cluster_t* cluster = calloc(1, sizeof(*cluster));
	if (cluster)
		cluster->vector = vector_create(0, address_destroy);
}

void cluster_destroy(void* data) {

	if (data) {
		cluster_t* cluster = data;
		vector_destroy(cluster->vector);
	}
}
