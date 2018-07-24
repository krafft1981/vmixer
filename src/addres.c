
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "addres.h"

struct address_s {

	char url[128];
	char host[128];
	char proto[10];
	int port;
};

address_t* address_create(const char* url) {

	if (!url)
		return NULL;

	address_t* address = malloc(sizeof(*address));
	if (!address)
		return NULL;

	char buffer [strlen(url) + 1];
	strcpy(buffer, url);

	char* ptr;
	char* token = strtok_r(buffer, ":", &ptr);

	if (!token)
		goto address_error;

	strncpy(address->proto, token, sizeof(address->proto));
	strncpy(address->url, url, sizeof(address->url));

	if (!strcmp(token, "unix")) { // local io

		if (ptr) {
			strncpy(address->host, ptr, sizeof(address->host));
			return address;
		}

		goto address_error;
	}

	int id = 1;
	while (token = strtok_r(NULL, ":", &ptr)) { // no local io
		id ++;
		switch (id) {
			case 2: {

				if (inet_addr(token) == -1)
					goto address_error;

				if (!strcmp(token, "0.0.0.0"))
					strncpy(address->host, "127.0.0.1", sizeof(address->host));
				else	strncpy(address->host, token, sizeof(address->host));
				break;
			}

			case 3: {
				address->port = atoi(token);
				if ((address->port == 0) || (address->port > 65535))
					goto address_error;
				break;
			}

			default:
				break;
		}
	}

	if (id < 3)
		goto address_error;

	return address;

address_error:
	address_destroy(address);
	return NULL;
}

void address_destroy(void* data) {

	if (data)
		free(data);
}

const char* address_get_url(address_t* address) {

	if (address)
		return address->url;

	return NULL;
}

const char* address_get_host(address_t* address) {

	if (address)
		return address->host;

	return NULL;
}

const char* address_get_proto(address_t* address) {

	if (address)
		return address->proto;

	return NULL;
}

int address_get_port(address_t* address) {

	if (address)
		return address->port;

	return -1;
}
