
#if HAVE_LIBNCAP
void Ncap_init(const char *device, int promisc);
int Ncap_run(DMC *, IPC *);
void Ncap_close(void);
int Ncap_start_time(void);
int Ncap_finish_time(void);
#endif
