

typedef struct _md_array md_array;
typedef struct _md_array_printer md_array_printer;
typedef struct _md_array_list md_array_list;
typedef struct _filter_list filter_list;
typedef struct _filter_defn FLTR;

typedef int (filter_func) (const void *message, const void *context);

typedef struct { 
    const char *name;
    int (*index_fn) (const void *);
    int (*iter_fn) (char **);
    void (*reset_fn) (void);
} indexer_t;

struct _filter_defn {
	const char *name;
	filter_func *func;
	const void *context;
};

struct _filter_list {
	FLTR *filter;
	struct _filter_list *next;
};

struct _md_array_node {
    int alloc_sz;
    int *array;
};

struct _md_array {
    const char *name;
    filter_list *filter_list;
    struct {
	indexer_t *indexer;
	const char *type;
	int alloc_sz;
    } d1;
    struct {
	indexer_t *indexer;
	const char *type;
	int alloc_sz;
    } d2;
    dataset_opt opts;
    struct _md_array_node *array;
};

struct _md_array_printer {
    void (*start_array) (void *, const char *);
    void (*finish_array) (void *);
    void (*d1_type) (void *, const char *);
    void (*d2_type) (void *, const char *);
    void (*start_data) (void *);
    void (*finish_data) (void *);
    void (*d1_begin) (void *, char *);
    void (*d1_end) (void *, char *);
    void (*print_element) (void *, char *label, int);
};

struct _md_array_list {
	md_array *theArray;
	md_array_list *next;
};

void md_array_clear(md_array *);
int md_array_count(md_array *, const void *);
md_array *md_array_create(const char *name, filter_list *,
    const char *, indexer_t *, const char *, indexer_t *);
int md_array_print(md_array * a, md_array_printer * pr, FILE *fp);
filter_list ** md_array_filter_list_append(filter_list **fl, FLTR *f);
FLTR * md_array_create_filter(const char *name, filter_func *, const void *context);
