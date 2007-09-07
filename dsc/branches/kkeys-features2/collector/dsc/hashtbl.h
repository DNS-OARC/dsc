
typedef struct _hashitem {
	const void *key;
	void *data;
	struct _hashitem *next;
} hashitem;

typedef unsigned int hashfunc(const void *key);
typedef int hashkeycmp(const void *a, const void *b);
typedef void hashfree(void *p);

typedef struct {
	unsigned int modulus;
	hashitem **items;
	hashfunc *hasher;
	hashkeycmp *keycmp;
	int use_arena;
	hashfree *keyfree;
	hashfree *datafree;
	struct {
		hashitem *next;
		unsigned int slot;
	} iter;
} hashtbl;


hashtbl *hash_create(int N, hashfunc *, hashkeycmp *, int use_arena,
    hashfree *, hashfree *);
void hash_destroy(hashtbl *);
int hash_add(const void *key, void *data, hashtbl *);
void hash_remove(const void *key, hashtbl *tbl);
void *hash_find(const void *key, hashtbl *);
void hash_iter_init(hashtbl *);
void *hash_iterate(hashtbl *);


/*
 * found in lookup3.c
 */
extern uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
extern uint32_t hashbig(const void *key, size_t length, uint32_t initval);
extern uint32_t hashword(const uint32_t *k, size_t length, uint32_t initval);

#ifndef BYTE_ORDER
#define hashendian hashlittle
#elif BYTE_ORDER == BIG_ENDIAN
#define hashendian hashbig
#else
#define hashendian hashlittle
#endif
