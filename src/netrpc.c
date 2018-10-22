
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <ev.h>

#include "parser.h"
#include "logger.h"
#include "netrpc.h"

#define LISTEN_COUNT 5

struct rpc_context_s {

	void* data;
	rpc_server_t* server;
	struct {
		int code;
		char* message;
	} error;
};

struct rpc_connection_s {

	int fd;
	struct ev_io io;
	rpc_server_t* server;
	struct {
		char* str;
		unsigned int size;
	} buffer;
};

struct rpc_server_s {

	struct ev_loop* loop;
	ev_io watcher;

	rbtree_t* proc;
	void* data;

	struct {
		struct sockaddr_in addr;
		int fd;
	} sock;

	struct {
		pthread_mutex_t mutex;
		pthread_cond_t cond;
	} td;
};

int rpc_register_procedure(rpc_server_t* server, rpc_method_f method, char* name) {

	if (!server || !name)
		return -1;

	return set_to_rbtree(server->proc, name, method);
}

int rpc_deregister_procedure(rpc_server_t *server, char* name) {

	if (!server || !name)
		return -1;

	return delete_from_rbtree(server->proc, name);
}

static int send_response(rpc_connection_t* conn, char* response) {

	DEBUG("JSON Response: '%s'", response);
	write(conn->fd, response, strlen(response));
	write(conn->fd, "\n", 1);
	return 0;
}

static int send_error(rpc_connection_t* conn, int code, char* message, json_node_t* id) {

	json_node_t* root = json_node_object(NULL);
	json_node_t* error = json_node_object(NULL);
	json_node_object_add(error, "code", json_node_int(code));
	json_node_object_add(error, "message", json_node_string(message));
	json_node_object_add(root, "error", error);
	json_node_object_add(root, "id", id);

	char str[1024 * 1024] = { 0 };
	int len = sizeof(str);

	if (json_node_print(root, JSON_STYLE_MINIMAL, &len, str))
		send_response(conn, str);
	json_node_destroy(root);
	return 0;
}

static int send_result(rpc_connection_t* conn, json_node_t* result, json_node_t* id) {

	json_node_t* root = json_node_object(NULL);
	if ( root)
		json_node_object_add(root, "result", result);
	json_node_object_add(root, "id", id);

	char str[1024 * 1024] = { 0 };
	int len = sizeof(str);

	if (json_node_print(result, JSON_STYLE_MINIMAL, &len, str))
		send_response(conn, str);
	json_node_destroy(root);
	return 0;
}

static int invoke_procedure(rpc_server_t* server, rpc_connection_t* conn, char* name, json_node_t* params, json_node_t* node) {

	rpc_context_t ctx = { 0 };
	rpc_method_f method = get_from_rbtree(server->proc, name);
	if (!method)
		return send_error(conn, RPC_METHOD_NOT_FOUND, "Method not found.", node);

	else {
//		if (ctx->error.code)
//			return send_error(conn, ctx->error.code, ctx->error.message, node);
//		else	return send_result(conn, ctx->function(&ctx, params, node), node);
	}
}

static int eval_request(rpc_server_t *server, rpc_connection_t* conn, json_node_t *root) {

	json_node_t *method;
	if (method = json_node_object_node(root, "method", JSON_NODE_TYPE_STRING)) {
		json_node_t* params = json_node_object_node(root, "params", JSON_NODE_TYPE_ANY);
		if (!params || json_node_type(params) == JSON_NODE_TYPE_ARRAY || json_node_type(params) == JSON_NODE_TYPE_OBJECT) {
			json_node_t* id = json_node_object_node(root, "id", JSON_NODE_TYPE_ANY);
			if (id == NULL|| json_node_type(id) == JSON_NODE_TYPE_STRING || json_node_type(id) == JSON_NODE_TYPE_INTEGER) {
				if (id != NULL)
					return invoke_procedure(server, conn, (char*)json_node_string_value(method), params, id);
			}
		}
	}

	send_error(conn, RPC_INVALID_REQUEST, "The JSON sent is not a valid Request object.", NULL);
	return -1;
}

static void close_connection(struct ev_loop* loop, ev_io* w) {

	ev_io_stop(loop, w);
	rpc_connection_t* conn = (rpc_connection_t*)w;
	close(conn->fd);
	free(conn->buffer.str);
	free(w);
}

static void connection_cb(struct ev_loop* loop, ev_io* io, int revents) {

	rpc_server_t* server = (rpc_server_t*) io->data;
	size_t readed = 0;
	rpc_connection_t* conn = (rpc_connection_t*) io;
	int fd = conn->fd;
/*
	if (conn->pos == (conn->size - 1)) {
		char* buffer = realloc(conn->buffer, conn->size *= 2);
		if (buffer == NULL)
			return close_connection(loop, w);
		conn->buffer = buffer;
		memset(conn->buffer + conn->pos, 0, conn->size - conn->pos);
	}

	int max_read_size = conn->size - conn->pos - 1;
	if ((readed = read(fd, conn->buffer + conn->pos, max_read_size)) == -1)
		return close_connection(loop, w);

	if (!readed)
		return close_connection(loop, w);

	else {
		json_node_t *root;
		conn->pos += readed;
		if ((root = parser_parse_buffer(NULL, conn->buffer, 10)) != NULL) {

			if (json_node_type(root) == JSON_NODE_TYPE_OBJECT)
				eval_request(server, conn, root);

			json_node_destroy(root);
		}
		else {
			// did we parse the all buffer? If so, just wait for more.
			// else there was an error before the buffer's end
//			if (end_ptr != (conn->buffer + conn->pos)) {
				send_error(conn, RPC_PARSE_ERROR, "Parse error. Invalid JSON was received by the server.", NULL);
				return close_connection(loop, w);
//			}
		}
	}
*/
}

static void accept_cb(struct ev_loop* loop, ev_io* io, int revents) {

	rpc_connection_t* conn = malloc(sizeof(*conn));
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(addr);

	conn->fd = accept(io->fd, (struct sockaddr*) &addr, &socklen);

	if (conn->fd == -1)
		free(conn);

	else {
		ev_io_init(&conn->io, connection_cb, conn->fd, EV_READ);
		conn->io.data = io->data;
		conn->buffer.size = 1500;
		conn->buffer.str = calloc(1, 1500);
		conn->server;
		ev_io_start(loop, &conn->io);
	}
}

rpc_server_t* rpc_server_create(char* addr, int port, void* data) {

	rpc_server_t* server = calloc(1, sizeof(*server));
	if (!server)
		return NULL;

	server->data = data;
	server->loop = EV_DEFAULT;
	server->proc = rbtree_create(NULL, NULL);
	server->sock.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	server->sock.addr.sin_family = AF_INET;
	server->sock.addr.sin_port = htons (port);

	pthread_mutex_init(&server->td.mutex, NULL);
	pthread_cond_init(&server->td.cond, NULL);

	int opt = 1;
	if (setsockopt (server->sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) == -1)
		goto tcp_server_error;

	inet_pton (AF_INET, addr, &server->sock.addr.sin_addr.s_addr);

	if (bind (server->sock.fd, (struct sockaddr *) &server->sock, sizeof (struct sockaddr_in)) == -1)
		goto tcp_server_error;

	if (listen (server->sock.fd, LISTEN_COUNT) == -1)
		goto tcp_server_error;

	ev_io_init(&server->watcher, accept_cb, server->sock.fd, EV_READ);
	server->watcher.data = server;
	ev_io_start(server->loop, &server->watcher);
	return server;

tcp_server_error:
	ERROR("%s", strerror(errno));
	if (server->sock.fd != -1)
		close(server->sock.fd);

	rbtree_destroy(server->proc);
	pthread_mutex_destroy(&server->td.mutex);
	pthread_cond_destroy(&server->td.cond);

	free(server);
	return NULL;
}

// Make the code work with both the old (ev_loop/ev_unloop)
// and new (ev_run/ev_break) versions of libev.
#ifdef EVUNLOOP_ALL
  #define EV_RUN ev_loop
  #define EV_BREAK ev_unloop
  #define EVBREAK_ALL EVUNLOOP_ALL
#else
  #define EV_RUN ev_run
  #define EV_BREAK ev_break
#endif

void rpc_server_run(rpc_server_t *server){

	EV_RUN(server->loop, 0);
}

int rpc_server_stop(rpc_server_t *server) {

	EV_BREAK(server->loop, EVBREAK_ALL);
	return 0;
}

void rpc_server_destroy(rpc_server_t *server){

	if (!server)
		return;

	rbtree_destroy(server->proc);
	pthread_mutex_destroy(&server->td.mutex);
	pthread_cond_destroy(&server->td.cond);
	free(server);
}
