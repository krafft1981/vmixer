
#include "logger.h"
#include "netrpc.h"

json_node_t* say_hello(rpc_context_t* ctx, json_node_t* params, json_node_t* id) {

	json_node_t* root = json_node_object(NULL);
	json_node_object_add(root, "message", json_node_string("Hello!"));
	json_node_object_add(root, "Hello", json_node_string(ctx->data));

	return root;
}

json_node_t* exit_server(rpc_context_t* ctx, json_node_t* params, json_node_t* id) {

	json_node_t* root = json_node_object(NULL);
	json_node_object_add(root, "message", json_node_string("Bye!"));
	json_node_object_add(root, "Hello", json_node_string(ctx->data));

	return root;
}

int main(int argc, char* argv[]) {

	setConsoleLog(1);
	setDebugMode(1);

	rpc_server_t* server = rpc_server_create("127.0.0.1", 5000, "NULL");
	rpc_register_procedure(server, say_hello, "sayHello");
	rpc_register_procedure(server, exit_server, "exit");
	rpc_server_run(server);
	rpc_server_destroy(server);

	return 0;
}
