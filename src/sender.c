
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "parser.h"
#include "config.h"
#include "client.h"
#include "logger.h"

int main(int argc, char* argv[]) {

	setConsoleLog(1);
	setDebugMode(1);

	client_t* client = NULL;

	int stress = 0;
	int prints = 0;
	const char* module = NULL;
	const char* thread = NULL;
	const char* method = NULL;

	int argument;
	while ((argument = getopt (argc, argv, "prs:f:m:t:c:?h")) != -1) {
		switch (argument) {
			case 'm': { module = optarg; break; }
			case 't': { thread = optarg; break; }
			case 'c': { method = optarg; break; }
			case 's': {
				client_destroy(client);
				client = client_create(optarg);
				break;
			}

			case 'r': {
				stress = 1;
				break;
			}

			case 'p': {
				prints = 1;
				break;
			}

			case 'f': {
				if (!client)
					break;

				struct stat st;
				if (!stat(optarg, &st)) {
					if (st.st_size > IO_BUFFER_SIZE)
						break;

					int fd = open(optarg, O_RDONLY);
					if (!fd)
						break;

					char buffer[IO_BUFFER_SIZE];

					if (read(fd, buffer, st.st_size) == st.st_size) {
						parser_t* parser = parser_create();
						json_node_t* request = parser_parse_buffer(parser, buffer, st.st_size);
						parser_destroy(parser);
						do {
							json_node_t* answer = NULL;
							if (client_request(client, module, thread, method, request, &answer) == 0) {

								if (prints) {
									buffer[0] = '\0';
									int len = IO_BUFFER_SIZE;
									json_node_print(answer, JSON_STYLE_MINIMAL, &len, buffer);
									INFO("%s", buffer);
								}

								if (answer)
									json_node_destroy(answer);
							}
						} while (stress);
						json_node_destroy(request);
					}
					close(fd);
				}
				break;
			}
		}
	}

	if (client)
		client_destroy(client);

	return 0;
}
