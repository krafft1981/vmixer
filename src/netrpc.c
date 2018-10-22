
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

struct rpc_context_s {

	void* data;
	struct {
		int code;
		char* message;
	} error;
};

struct rpc_connection_s {

	struct ev_io io;
	int fd;
	int pos;
	unsigned int buffer_size;
	char* buffer;
	rpc_server_t* server;
};

struct rpc_server_s {

	struct ev_loop* loop;
	ev_io watcher;
	rbtree_t* procedures;
	struct sockaddr_in sockaddr;
	pthread_mutex_t mutex;
};

static int send_response(rpc_connection_t* conn, char* response) {

	int fd = conn->fd;
	DEBUG("JSON Response:\n%s", response);
	write(fd, response, strlen(response));
	write(fd, "\n", 1);
	return 0;
}

static int send_error(rpc_connection_t* conn, int code, char* message, json_node_t* node) {

	json_node_t *result_root = json_node_object(NULL);
	json_node_t *error_root = json_node_object(NULL);
	json_node_object_add(error_root, "code", json_node_int(code));
	json_node_object_add(error_root, "message", json_node_string(message));
	json_node_object_add(result_root, "error", error_root);
	json_node_object_add(result_root, "id", node);
	char str[1024 * 1024] = { 0 };
	json_node_print(result_root, JSON_STYLE_MINIMAL, &len, str);
	json_node_destroy(result_root);
	send_response(conn, str);
	return 0;
}

static int send_result(rpc_connection_t* conn, json_node_t* result, json_node_t* node) {

	json_node_t *result_root = json_node_object(NULL);
	if (result)
		json_node_object_add(result_root, "result", result);
	json_node_object_add(result_root, "id", node);

	char str_result[10240];
//	 = json_node_Print(result_root);
	int return_value;// = send_response(conn, str_result);
	json_node_destroy(result_root);
	return return_value;
}

static int invoke_procedure(rpc_server_t* server, rpc_connection_t* conn, char* name, json_node_t* params, json_node_t* node) {

	rpc_context_t* ctx = get_from_rbtree(server->procedures, name);
	if (!ctx)
		return send_error(conn, RPC_METHOD_NOT_FOUND, "Method not found.", node);

	else {
		if (ctx->error.code)
			return send_error(conn, ctx->error.code, ctx->error.message, node);
//		else	return send_result(conn, ctx->function(&ctx, params, node), node);
	}
}

//	int i = rbtree_size(server->procedures);
//	while (i--) {
//		if (!strcmp(server->procedures[i].name, name)) {
//			procedure_found = 1;
//			ctx.data = server->procedures[i].data;
//			returned = server->procedures[i].function(&ctx, params, id);
//			break;
//		}
//	}
//	if (!procedure_found)
//		return send_error(conn, JRPC_METHOD_NOT_FOUND,
//				strdup("Method not found."), id);
//	else {
//		if (ctx.error_code)
//			return send_error(conn, ctx.error_code, ctx.error_message, id);
//		else	return send_result(conn, returned, id);
//	}

/*
static int eval_request(rpc_server_t *server,
		struct jrpc_connection * conn, json_node_t *root) {
	json_node_t *method, *params, *id;
	method = json_node_GetObjectItem(root, "method");
	if (method != NULL && method->type == json_node_String) {
		params = json_node_GetObjectItem(root, "params");
		if (params == NULL|| params->type == json_node_Array
		|| params->type == json_node_Object) {
			id = json_node_GetObjectItem(root, "id");
			if (id == NULL|| id->type == json_node_String
			|| id->type == json_node_Number) {
			//We have to copy ID because using it on the reply and deleting the response Object will also delete ID
				json_node_t * id_copy = NULL;
				if (id != NULL)
					id_copy =
							(id->type == json_node_String) ? json_node_CreateString(
									id->valuestring) :
									json_node_CreateNumber(id->valueint);
				if (server->debug_level)
					printf("Method Invoked: %s\n", method->valuestring);
				return invoke_procedure(server, conn, method->valuestring,
						params, id_copy);
			}
		}
	}
	send_error(conn, RPC_INVALID_REQUEST,
			strdup("The JSON sent is not a valid Request object."), NULL);
	return -1;
}
*/

static void close_connection(struct ev_loop *loop, ev_io *w) {
	ev_io_stop(loop, w);
	close(((struct rpc_connection_t*) w)->fd);
	free(((struct rpc_connection_t*) w)->buffer);
	free(((struct rpc_connection_t*) w));
}

static void connection_cb(struct ev_loop *loop, ev_io *w, int revents) {

	struct rpc_connection *conn;
	rpc_server_t* server = (rpc_server_t*) w->data;
	size_t bytes_read = 0;
	//get our 'subclassed' event watcher
	conn = (struct rpc_connection*) w;
	int fd = conn->fd;
	if (conn->pos == (conn->buffer_size - 1)) {
		char * new_buffer = realloc(conn->buffer, conn->buffer_size *= 2);
		if (new_buffer == NULL) {
			perror("Memory error");
			return close_connection(loop, w);
		}
		conn->buffer = new_buffer;
		memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos);
	}
	// can not fill the entire buffer, string must be NULL terminated
	int max_read_size = conn->buffer_size - conn->pos - 1;
	if ((bytes_read = read(fd, conn->buffer + conn->pos, max_read_size)) == -1) {
		perror("read");
		return close_connection(loop, w);
	}

	if (!bytes_read) {
		// client closed the sending half of the connection
		INFO("Client closed connection.\n");
		return close_connection(loop, w);
	}

	else {
		json_node_t *root;
		char *end_ptr = NULL;
		conn->pos += bytes_read;

		if ((root = json_node_Parse_Stream(conn->buffer, &end_ptr)) != NULL) {
			char * str_result = json_node_Print(root);
			INFO("Valid JSON Received:\n%s\n", str_result);
			free(str_result);

			if (json_node_type(root) == JSON_NODE_TYPE_OBJECT)
				eval_request(server, conn, root);

			//shift processed request, discarding it
			memmove(conn->buffer, end_ptr, strlen(end_ptr) + 2);

			conn->pos = strlen(end_ptr);
			memset(conn->buffer + conn->pos, 0,
					conn->buffer_size - conn->pos - 1);

			json_node_destroy(root);
		}
		else {
			// did we parse the all buffer? If so, just wait for more.
			// else there was an error before the buffer's end
			if (end_ptr != (conn->buffer + conn->pos)) {
				printf("INVALID JSON Received:\n---\n%s\n---\n", conn->buffer);
				send_error(conn, RPC_PARSE_ERROR, "Parse error. Invalid JSON was received by the server.", NULL);
				return close_connection(loop, w);
			}
		}
	}
}

/*
static void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*) sa)->sin_addr);
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

static void accept_cb(struct ev_loop *loop, ev_io *w, int revents) {
	char s[INET6_ADDRSTRLEN];
	struct jrpc_connection *connection_watcher;
	connection_watcher = malloc(sizeof(struct jrpc_connection));
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	sin_size = sizeof their_addr;
	connection_watcher->fd = accept(w->fd, (struct sockaddr *) &their_addr,
			&sin_size);
	if (connection_watcher->fd == -1) {
		perror("accept");
		free(connection_watcher);
	} else {
		if (((rpc_server_t *) w->data)->debug_level) {
			inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);
		}
		ev_io_init(&connection_watcher->io, connection_cb,
				connection_watcher->fd, EV_READ);
		//copy pointer to rpc_server_t
		connection_watcher->io.data = w->data;
		connection_watcher->buffer_size = 1500;
		connection_watcher->buffer = malloc(1500);
		memset(connection_watcher->buffer, 0, 1500);
		connection_watcher->pos = 0;
		//copy debug_level, struct jrpc_connection has no pointer to rpc_server_t
		connection_watcher->debug_level =
				((rpc_server_t *) w->data)->debug_level;
		ev_io_start(loop, &connection_watcher->io);
	}
}
*/

rpc_server_t* rpc_server_create(char* addr, int port) {

	rpc_server_t* server = calloc(1, sizeof(*server));
	if (server) {
		server->loop = EV_DEFAULT;
		setDebugMode(10);
		setConsoleLog(10);

		char * debug_level_env = getenv("RPC_DEBUG");
		if (!debug_level_env)
			setDebugMode(0);
		else {
			setDebugMode(strtol(debug_level_env, NULL, 10));
			DEBUG("RPC Debug level %d", getDebugMode());
		}

//		return __jrpc_server_start(server);

		int sockfd;
		struct addrinfo hints, *servinfo, *p;
		unsigned int len;
		int yes = 1;
		int rv;
		char PORT[6];
		sprintf(PORT, "%d", port);
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP

	}
	return server;
}
/*
static int __jrpc_server_start(rpc_server_t *server) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in sockaddr;
	unsigned int len;
	int yes = 1;
	int rv;
	char PORT[6];
	sprintf(PORT, "%d", server->port_number);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		len = sizeof(sockaddr);
		if (getsockname(sockfd, (struct sockaddr *) &sockaddr, &len) == -1) {
			close(sockfd);
			perror("server: getsockname");
			continue;
		}
		server->port_number = ntohs( sockaddr.sin_port );

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	if (server->debug_level)
		printf("server: waiting for connections...\n");

	ev_io_init(&server->listen_watcher, accept_cb, sockfd, EV_READ);
	server->listen_watcher.data = server;
	ev_io_start(server->loop, &server->listen_watcher);
	return 0;
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

void jrpc_server_run(rpc_server_t *server){
	EV_RUN(server->loop, 0);
}

int jrpc_server_stop(rpc_server_t *server) {
	EV_BREAK(server->loop, EVBREAK_ALL);
	return 0;
}

void jrpc_server_destroy(rpc_server_t *server){

	int i;
	for (i = 0; i < server->procedure_count; i++){
		jrpc_procedure_destroy( &(server->procedures[i]) );
	}
	free(server->procedures);
}

static void jrpc_procedure_destroy(struct jrpc_procedure *procedure){
	if (procedure->name){
		free(procedure->name);
		procedure->name = NULL;
	}
	if (procedure->data){
		free(procedure->data);
		procedure->data = NULL;
	}
}

int jrpc_register_procedure(rpc_server_t *server,
		jrpc_function function_pointer, char *name, void * data) {
	int i = server->procedure_count++;
	if (!server->procedures)
		server->procedures = malloc(sizeof(struct jrpc_procedure));
	else {
		struct jrpc_procedure * ptr = realloc(server->procedures,
				sizeof(struct jrpc_procedure) * server->procedure_count);
		if (!ptr)
			return -1;
		server->procedures = ptr;

	}
	if ((server->procedures[i].name = strdup(name)) == NULL)
		return -1;
	server->procedures[i].function = function_pointer;
	server->procedures[i].data = data;
	return 0;
}

int jrpc_deregister_procedure(rpc_server_t *server, char *name) {

	int i;
	int found = 0;
	if (server->procedures){
		for (i = 0; i < server->procedure_count; i++){
			if (found)
				server->procedures[i-1] = server->procedures[i];
			else if(!strcmp(name, server->procedures[i].name)){
				found = 1;
				jrpc_procedure_destroy( &(server->procedures[i]) );
			}
		}
		if (found){
			server->procedure_count--;
			if (server->procedure_count){
				struct jrpc_procedure * ptr = realloc(server->procedures,
					sizeof(struct jrpc_procedure) * server->procedure_count);
				if (!ptr){
					perror("realloc");
					return -1;
				}
				server->procedures = ptr;
			}else{
				server->procedures = NULL;
			}
		}
	} else {
		fprintf(stderr, "server : procedure '%s' not found\n", name);
		return -1;
	}
	return 0;
}
*/
