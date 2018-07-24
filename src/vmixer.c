
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "config.h"
#include "propes.h"
#include "client.h"
#include "thread.h"
#include "vector.h"
#include "rbtree.h"
#include "addres.h"
#include "loader.h"
#include "logger.h"

#define LISTEN_COUNT 5

typedef struct server_s server_t;
typedef struct connect_s connect_t;

struct connect_s {

	int sock;
	struct sockaddr_in client;

	server_t* server;
	parser_t* parser;

	struct {
		loader_t* loader;
		thread_t* thread;
		method_t* method;
	} target;

	struct {
		int count;
		int reqst;
	} stat;
};

struct server_s {

	int sock;

	pthread_t td;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

	address_t* address;
	rbtree_t* loader;

	struct {
		int count;
	} stat;

	char* confdir;
};

void target_request(connect_t* conn, server_t* server, json_node_t* request, json_node_t* answer) {

	if (!conn || !server || !request || !answer)
		return;

	json_node_t* target = json_node_object_node(request, "target", JSON_NODE_TYPE_OBJECT);
	json_node_t* args   = json_node_object_node(request, "args"  , JSON_NODE_TYPE_ANY);

	json_node_t* module = json_node_object_node(target, "module", JSON_NODE_TYPE_STRING);
	json_node_t* thread = json_node_object_node(target, "thread", JSON_NODE_TYPE_STRING);
	json_node_t* method = json_node_object_node(target, "method", JSON_NODE_TYPE_STRING);

	if (module) {
		if (thread) {
			if (method) {
				if (!(conn->target.loader = get_from_rbtree(conn->server->loader, json_node_string_value(module))))
					json_node_object_add(answer, "error", json_node_string("module not found"));

				else {
					locker_set(loader_locker(conn->target.loader), THREAD_LOCK_READ);
					if (!(conn->target.thread = get_from_loader(conn->target.loader, json_node_string_value(thread))))
						json_node_object_add(answer, "error", json_node_string("thread not found"));

					else {
						if (!(conn->target.method = thread_method(conn->target.thread, json_node_string_value(method))))
							json_node_object_add(answer, "error", json_node_string("method not found"));

						else {
							if (conn->target.method->run)
								conn->target.method->run(conn->target.thread, args, answer);
						}
					}

					locker_set(loader_locker(conn->target.loader), THREAD_UNLOCK_READ);
				}
			}
			else {
				json_node_object_add(answer, "info", json_node_string("thread command"));
			}
		}
		else {
			json_node_object_add(answer, "info", json_node_string("module command"));
		}
	}
	else {
		json_node_object_add(answer, "info", json_node_string("kernel command"));
	}
}

void connect_thread(void* data) {

	connect_t conn = *(connect_t*)data;
	DEBUG("client %d connected", conn.stat.count);

	pthread_mutex_lock(&conn.server->mutex);
	pthread_cond_broadcast(&conn.server->cond);
	pthread_mutex_unlock(&conn.server->mutex);

	char buffer[IO_BUFFER_SIZE];

	while (1) {
		conn.stat.reqst ++;

		size_t size;
		if (read(conn.sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			break;

		int readed = 0;
		while (readed != size) {
			int msgsize = read(conn.sock, &buffer[readed], size - readed);
			if (msgsize <= 0)
				break;
			readed += msgsize;
		}

		if (readed != size)
			break;

		json_node_t* request = parser_parse_buffer(conn.parser, buffer, size);
		json_node_t* answer = json_node_object(NULL);
		target_request(&conn, conn.server, request, answer);
		json_node_destroy(request);

		size = IO_BUFFER_SIZE;
		if (IO_BUFFER_SIZE)
			buffer[0] = '\0';

		if (json_node_print(answer, JSON_STYLE_MINIMAL, (int*)&size, buffer)) {
			json_node_destroy(answer);
			break;
		}

		json_node_destroy(answer);
		size = IO_BUFFER_SIZE - size;

		if (write(conn.sock, &size, HEADER_MSG_SIZE) != HEADER_MSG_SIZE)
			break;

		if (write(conn.sock, buffer, size) != size)
			break;
	}

	DEBUG("client %d disconnected", conn.stat.count);

	parser_destroy(conn.parser);
	close(conn.sock);
}

int main (int argc, char *argv[]) {

	signal(SIGPIPE, SIG_IGN);

	server_t server = {
		.cond    = PTHREAD_COND_INITIALIZER,
		.mutex   = PTHREAD_MUTEX_INITIALIZER,
		.loader  = rbtree_create(NULL, loader_destroy),
		.sock    = 0,
		.stat    = { 0 },
		.confdir = NULL,
		.address = NULL,
	};

	int argument;
	while ((argument = getopt (argc, argv, "b:p:m:c:U:G:l:?h")) != -1) {
		switch (argument) {

			case 'b': {
				if (server.sock > 0)
					close(server.sock);
				server.sock = 0;
				if (!(server.address = address_create(optarg)))
					break;
#ifdef ENABLE_TCP
				if (!strcmp(address_get_proto(server.address), "tcp")) {
					server.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					int opt = 1;
					if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) == -1)
						goto tcp_server_final;

					struct sockaddr_in srv = { 0 };
					srv.sin_family = AF_INET;
					srv.sin_port = htons (address_get_port(server.address));
					inet_pton (AF_INET, address_get_host(server.address), &srv.sin_addr.s_addr);
					if (bind (server.sock, (struct sockaddr *) &srv, sizeof (struct sockaddr_in)) == -1)
						goto tcp_server_final;

					if (listen (server.sock, LISTEN_COUNT) == -1)
						goto tcp_server_final;
					break;

					tcp_server_final:
					ERROR ("server at '%s': %s", address_get_url(server.address), strerror (errno));
					if (server.sock > 0)
						close(server.sock);

					return -1;
				}
#endif
#ifdef ENABLE_UNIX
				if (!strcmp(address_get_proto(server.address), "unix")) {
					server.sock = socket(AF_LOCAL, SOCK_STREAM, 0);
					unlink(address_get_host(server.address));
					struct sockaddr_un srv = { 0 };
					srv.sun_family = AF_LOCAL;
					strncpy (srv.sun_path, address_get_host(server.address), sizeof (srv.sun_path));
					socklen_t socksize = sizeof (srv.sun_path);
					if (bind (server.sock, (struct sockaddr *) &srv, socksize) == -1)
						goto unix_server_final;

					if (listen (server.sock, LISTEN_COUNT) == -1)
						goto unix_server_final;

					break;

					unix_server_final:
					ERROR ("server at '%s': %s", address_get_url(server.address), strerror (errno));
					if (server.sock > 0)
						close(server.sock);

					return -1;
				}
#endif
#ifdef ENABLE_SCTP
				if (!strcmp(address_get_proto(server.address), "sctp")) {
					server.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
					int opt = 1;
					if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
						goto sctp_server_final;

					struct sockaddr_in srv = { 0 };
					srv.sin_family = AF_INET;
					srv.sin_port = htons(address_get_port(server.address));
					inet_pton (AF_INET, address_get_host(server.address), &srv.sin_addr.s_addr);
					if (bind (server.sock, (struct sockaddr *) &srv, sizeof (struct sockaddr_in)) == -1)
						goto sctp_server_final;

					if (listen (server.sock, LISTEN_COUNT) == -1)
						goto sctp_server_final;
					break;

					sctp_server_final:
					ERROR ("server at '%s': %s", address_get_url(server.address), strerror (errno));
					if (server.sock > 0)
						close(server.sock);

					return -1;
				}
#endif
#ifdef ENABLE_UDP
				if (!strcmp(address_get_proto(server.address), "udp")) {
					server.sock = socket(AF_INET, SOCK_DGRAM, 0);
					int opt = 1;
					if (setsockopt (server.sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) == -1)
						goto udp_server_final;

					struct sockaddr_in srv = { 0 };
					srv.sin_family = AF_INET;
					srv.sin_port = htons (address_get_port(server.address));
					inet_pton (AF_INET, address_get_host(server.address), &srv.sin_addr.s_addr);
					if (bind (server.sock, (struct sockaddr *) &srv, sizeof (struct sockaddr_in)) == -1)
						goto udp_server_final;

					break;

					udp_server_final:
					ERROR ("server at '%s': %s", address_get_url(server.address), strerror (errno));
					if (server.sock > 0)
						close(server.sock);

					return -1;
				}
#endif
				ERROR("protocol '%s' not compiled", address_get_proto(server.address));
				if (server.loader)
					rbtree_destroy(server.loader);

				return -1;
			}

			case 'c': {
				server.confdir = optarg;
				break;
			}

			case 'U': { // set process user
				struct passwd *uid = getpwnam(optarg);
				if (uid) {
					if (setuid(uid->pw_uid))
						printf("set user id error: %s\n", strerror(errno));
					else	printf("set user id: %s\n", optarg);
				}
				else
					printf("can't get uid from user: %s\n", optarg);
				break;
			}

			case 'G': { // set process group
				struct group *gid = getgrnam(optarg);
				if (gid) {
					if (setgid(gid->gr_gid))
						printf("set group id error: %s\n", strerror(errno));
					else	printf("set group id: %s\n", optarg);
				}
				else
					printf("can't get gid from group: %s\n", optarg);
				break;
			}

			case 'p': { // daemon PID file
				FILE *fp = fopen(optarg, "w");
				if (fp) {
					fprintf(fp, "%d", getpid ());
					fclose(fp);
				}
				else
					printf("can't create pid file\n");
				break;
			}

			case 'm': {
				switch (atoi(optarg)) {
					case 0: { // init child
						setConsoleLog(1);
						setDebugMode(0);
						break;
					}

					case 1: { // debug via concole
						setConsoleLog(1);
						setDebugMode(1);
						break;
					}

					case 2: { // daemon
						if (daemon(1,1))
							ERROR("daemon: %s\n", strerror(errno));
						else {
							setConsoleLog(0);
							setDebugMode(0);
						}

						break;
					}

					default: { // uncknown mode
						fprintf(stderr, "uncknown mode: %s\n", optarg);
						break;
					}
				}

				break;
			}

			case 'l': {

				loader_t* loader = loader_create(optarg);
				if (loader)
					set_to_rbtree(server.loader, loader_module(loader)->name, loader);
				break;
			}

			case '?':
			case 'h':
			default:
				printf("usage: %s [-U user][-G group][-p pid][-l module][-b bindig][-c confdir][-m mode]\n", argv[0]);
		}
	}

	if (!server.address) {
		rbtree_destroy(server.loader);
		return -1;
	}

	if (server.confdir) {
		DIR *dir = opendir(server.confdir);
		if (dir) {
			connect_t conn = {
				.server = &server,
				.parser = parser_create(),
				.client = { 0 },
				.target = { 0 },
				.stat.count = server.stat.count ++,
			};

			struct dirent *entry;

			while ((entry = readdir(dir)) != NULL) {
				if (entry->d_type == DT_REG) {
					char name[PATH_MAX];
					snprintf(name, PATH_MAX, "%s/%s", server.confdir, entry->d_name);
					json_node_t* request = parser_parse_file(conn.parser, name);
					if (request) {
						json_node_t* answer = json_node_object(NULL);
						target_request(&conn, &server, request, answer);
						json_node_destroy(request);
						json_node_destroy(answer);
					}
				}
			}
			parser_destroy(conn.parser);
			closedir(dir);
		}
	}

	INFO("server started at '%s'", address_get_url(server.address));

	if (!strcmp(address_get_proto(server.address), "udp")) {
		while(1) {
			struct timeval timer = {.tv_sec = 1, .tv_usec = 0 };

			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET (server.sock, &rfds);

			if (select (server.sock + 1, &rfds, NULL, NULL, &timer) > 0) {
				char buffer [IO_BUFFER_SIZE];
				socklen_t len = sizeof(buffer);
				struct sockaddr_in client = { 0 };
				int msgsize = recvfrom(server.sock, buffer, sizeof (buffer), 0, (struct sockaddr *) &client, &len);
				ERROR("proptocol not ready yet !!!");
			}
		}
	}

	else {
		while(1) {
			struct timeval timer = {.tv_sec = 1, .tv_usec = 0 };

			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET (server.sock, &rfds);

			if (select (server.sock + 1, &rfds, NULL, NULL, &timer) > 0) {
				connect_t conn = {
					.server = &server,
					.parser = parser_create(),
					.client = { 0 },
					.stat.count = server.stat.count ++,
				};

				int optarg = 1;
				socklen_t optlen = sizeof(conn.client);

				if ((conn.sock = accept(server.sock, (struct sockaddr*)&conn.client, &optlen)) > 0) {
					setsockopt (conn.sock, SOL_SOCKET, SO_KEEPALIVE, &optarg, sizeof(optarg));

					pthread_t td;
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
					pthread_mutex_lock(&server.mutex);
					if (pthread_create(&td, &attr, (void*(*)(void*)) connect_thread, &conn))
						INFO ("%s", strerror (errno));
					else	pthread_cond_wait(&server.cond, &server.mutex);
					pthread_mutex_unlock(&server.mutex);
					pthread_attr_destroy(&attr);
				}

				else
					close(conn.sock);
			}
		}
	}

	rbtree_destroy(server.loader);
	return 0;
}
