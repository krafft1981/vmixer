
#include <string.h>
#include "config.h"
#include "parser.h"
#include "addres.h"
#include "loader.h"
#include "logger.h"

module_t export = { 0 };

int main(int argc, char* argv[]) {

	setConsoleLog(1);
	setDebugMode(1);

	parser_t* parser = parser_create();

	int id = 1000;
	while (id --) {
		json_node_t* node = parser_parse_file(parser, argv[1]);
		if (node)
			json_node_destroy(node);
	}

	parser_destroy(parser);
	return 0;
}
