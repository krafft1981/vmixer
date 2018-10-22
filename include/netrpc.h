
#ifndef NETRPC_H
#define NETRPC_H

#include <parser.h>

#define RPC_PARSE_ERROR      -32700
#define RPC_INVALID_REQUEST  -32600
#define RPC_METHOD_NOT_FOUND -32601
#define RPC_INVALID_PARAMS   -32603
#define RPC_INTERNAL_ERROR   -32693

typedef struct rpc_context_s rpc_context_t;
typedef struct rpc_server_s rpc_server_t;
typedef struct rpc_connection_s rpc_connection_t;
typedef struct rpc_procedure_s rpc_procedure_t;

typedef json_node_t* (*rpc_method_f)(rpc_context_t* context, json_node_t* params, json_node_t* node);

/** */
rpc_server_t* rpc_server_create(char* addr, int port, void* data);

/** */
void rpc_server_run(rpc_server_t* server);

/** */
int rpc_server_stop(rpc_server_t* server);

/** */
void rpc_server_destroy(rpc_server_t* server);

/** */
int rpc_register_procedure(rpc_server_t* server, rpc_method_f method, char* name);

/** */
int rpc_deregister_procedure(rpc_server_t* server, char* name);

#endif
