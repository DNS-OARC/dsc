typedef struct _gen_cnt gen_cnt;
struct _gen_cnt {
    int cnt;
    void *ptr;
};

int gen_cnt_cmp(const void *A, const void *B);
