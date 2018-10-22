
#include "netrpc.h"

void* data;

json_node_t* say_hello(rpc_context_t* ctx, json_node_t* params, json_node_t* id) {

	return json_node_string("Hello!");
}

json_node_t* exit_server(rpc_context_t* ctx, json_node_t* params, json_node_t* id) {

	return json_node_string("Bye!");
}

int main(int argc, char* argv[]) {

	rpc_server_t* server = rpc_server_create("127.0.0.1", 5000, data);
	rpc_register_procedure(server, say_hello, "sayHello");
	rpc_register_procedure(server, exit_server, "exit");
	rpc_server_run(server);
	rpc_server_destroy(server);
}
