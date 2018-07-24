
#ifndef CLIENT_H
#define CLIENT_H

#include <addres.h>
#include <parser.h>

/** this structure are protected */
typedef struct client_s client_t;
/** this structure are protected */
typedef struct cluster_s cluster_t;

/** create client_t */
client_t* client_create(const char* url);

/** create client_t from cluster */
client_t* client_cluster(cluster_t* cluster);

/** destroy client_t */
void client_destroy(void* data);

/** client request */
int client_request(client_t* client, const char* module, const char* thread, const char* method, json_node_t* args, json_node_t* *answer);

/** client file request */
int client_file_request(client_t* client, const char* file, json_node_t* *answer);

/** create cluster_t */
cluster_t* cluster_create();

/** destroy cluster_t */
void cluster_destroy(void* data);

#endif // CLIENT_H
