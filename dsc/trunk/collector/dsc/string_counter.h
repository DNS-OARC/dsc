

typedef struct _StringCounter StringCounter;
struct _StringCounter {
    char *s;
    int count;
    StringCounter *next;
};

StringCounter * StringCounter_lookup_or_add(StringCounter **, const char *);
void StringCounter_sort(StringCounter **);
