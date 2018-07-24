
#ifndef ADDRES_H
#define ADDRES_H

/** protected structure*/
typedef struct address_s address_t;

/** create address_t struct */
address_t* address_create(const char* url);

/** destroy address_t struct */
void address_destroy(void* data);

/** get address url */
const char* address_get_url(address_t* address);

/** get address host */
const char* address_get_host(address_t* address);

/** get address proto */
const char* address_get_proto(address_t* address);

/** get address port */
int address_get_port(address_t* address);

#endif // ADDRES_H
