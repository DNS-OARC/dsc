

typedef struct _md_array md_array;
typedef struct _md_array_printer md_array_printer;

typedef int (IDXR) (dns_message *);
typedef int (HITR) (char **);

struct _md_array {
    struct {
	IDXR *indexer;
	HITR *iterator;
	char *type;
	int alloc_sz;
	int max_idx;
    } d1;
    struct {
	IDXR *indexer;
	HITR *iterator;
	char *type;
	int alloc_sz;
	int max_idx;
    } d2;
    int **array;
};

struct _md_array_printer {
	void (*start_array)(void);
	void (*finish_array)(void);
	void (*d1_type)(char *);
	void (*d2_type)(char *);
	void (*start_data)(void);
	void (*finish_data)(void);
	void (*d1_begin)(char *);
	void (*d1_end)(char *);
	void (*print_element)(char *label, int);
};

int md_array_count(md_array *, dns_message *);
md_array *md_array_create(char *, IDXR *, HITR *, char *, IDXR *, HITR *);
int md_array_print(md_array * a, md_array_printer * pr);
