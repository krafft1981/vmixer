
#include <stdio.h>
#include "netrpc.h"

int main(int argc, char* argv[]) {

	parser_t* parser = parser_create();
	json_node_t* node = parser_parse_string(parser, "{\"a\":\"b\", \"c\":\"d\"}");
	if (node) {
		char str[30] = {0};
		int len = sizeof(str);
		printf("res(%d) \"%s\"\n", json_node_print(node, JSON_STYLE_MINIMAL, &len, str), str);
		json_node_destroy(node);
	}
	parser_destroy(parser);
}
