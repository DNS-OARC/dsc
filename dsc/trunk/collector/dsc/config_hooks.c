#include <syslog.h>
#include <unistd.h>
#include <string.h>

int promisc_flag;
void Pcap_init(const char *device, int promisc);

int
open_interface(const char *interface)
{
        syslog(LOG_INFO, "Opening interface %s", interface);
        Pcap_init(interface, promisc_flag);
	return 1;
}

int
set_bpf_program(const char *s)
{
	extern char *bpf_program_str;
	bpf_program_str = strdup(s);
	return 1;
}


int
set_run_dir(const char *dir)
{
        syslog(LOG_INFO, "setting current directory to %s", dir);
        if (chdir(dir) < 0)
		return 0;
	return 1;
}

