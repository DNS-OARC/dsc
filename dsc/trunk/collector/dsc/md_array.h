

typedef struct _md_array md_array;
typedef struct _md_array_printer md_array_printer;
typedef struct _md_array_list md_array_list;
typedef struct _filter_list filter_list;

typedef int (IDXR) (const void *);
typedef int (HITR) (char **);
typedef int (FLTR) (const void *);

struct _filter_list {
	FLTR *filter;
	struct _filter_list *next;
};

struct _md_array {
    const char *name;
    filter_list *filter_list;
    struct {
	IDXR *indexer;
	HITR *iterator;
	const char *type;
	int alloc_sz;
    } d1;
    struct {
	IDXR *indexer;
	HITR *iterator;
	const char *type;
	int alloc_sz;
    } d2;
    struct {
	int min_count;
    } opts;
    int **array;
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
    void (*print_element) (void *, char *label, int, int);
};

struct _md_array_list {
	md_array *theArray;
	md_array_list *next;
};

int md_array_count(md_array *, const void *);
md_array *md_array_create(const char *name, filter_list *,
    const char *, IDXR *, HITR *,
    const char *, IDXR *, HITR *);
int md_array_print(md_array * a, md_array_printer * pr);
filter_list ** md_array_filter_list_append(filter_list **fl, FLTR *f);
