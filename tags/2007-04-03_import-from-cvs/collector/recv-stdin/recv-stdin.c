#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <md5.h>
#include <err.h>
#include <string.h>

/*
 * PV 2005-09-26
 * if there were a "dsc" command, ideally implemented in C rather than Perl,
 * and it took one argument which was a "preferred file name" (no /'s allowed),
 * and it copied its stdin to a file by that name in a "dsc/" subdirectory of
 * login/home directory, and emitted an MD5 back (toward the remote ssh client)
 * so that the sender would have some confidence that deletion was OK... then
 * we could conceivably give each member a "dsc/" symlink to some common dsc
 * spool directory owned by a group all the members are in.
 */


int
main(int argc, char *argv[])
{
	char *fn;
	char buf[4096];
	int n;
	int m;
	int fd;
        MD5_CTX md5;
        char hash_str[33];

	if (argc != 2) {
		fprintf(stderr, "usage: %s filename\n", argv[0]);
		return 1;
	}

	fn = argv[1];

	if (strchr(fn, '/'))
		errx(1, "'/' is not allowed in filename");

	if (NULL == getenv("HOME"))
		errx(1, "Did not find $HOME in environment");
	if (chdir(getenv("HOME")) < 0)
		err(1, getenv("HOME"));
	if (chdir("dsc") < 0)
		err(1, "$HOME/dsc");

	fd = open(fn, O_WRONLY|O_CREAT|O_EXCL, 0660);
	if (fd < 0)
		err(1, fn);

	MD5Init(&md5);
	while ((n = read(0, buf,  4096)) > 0) {
		m = write(fd, buf, n);
		if (m != n)
			err(1, "write");
		MD5Update(&md5, buf, n);
	}
	close(fd);
	MD5End(&md5, hash_str);
	printf("%s\n", hash_str);

	return 0;
}

