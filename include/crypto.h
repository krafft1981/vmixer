
#ifndef CRYPTO_H
#define CRYPTO_H

typedef struct md5_state_s md5_state_t;

/** Initialize the algorithm. */
void md5_init(md5_state_t *pms);

/** Append a string to the message. */
void md5_append(md5_state_t *pms, const unsigned char *data, int nbytes);

/** Finish the message and return the digest. */
void md5_finish(md5_state_t *pms, unsigned char digest[16]);

#endif // CRYPTO_H
