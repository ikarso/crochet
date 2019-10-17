/*-
 * Copyright (c) 2019 Oskar Holmlund <oskar@ohdata.se>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Problem: crochet produce following error:
	pkg: Cannot runscript POST-INSTALL:Exec format error

   freebsd pkg info (src/info.c) was a little bit to complicated so this is a
   stripped down version just to extract the script part and write it to a file
*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <pkg.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>

void
usage_info(char *program)
{
	fprintf(stderr, "Usage: %s <pkg-file> <output directory>\n",
	    program);
}

int
write_pkg_script(struct pkg * const pkg, pkg_script type, char *filename)
{
	int fd;
	char buf[4096];
	ssize_t size;
        const char *script = pkg_script_get(pkg, type);
        if (script == NULL) {
                return 0;
        }

	/* Format content of the script */
	size = snprintf(buf, 4096, "#!/bin/sh\n%s", script);
	if (size > 4096) {
		fprintf(stderr, "To small buffer\n");
		return 0;
	}

	fd = open(filename, O_RDWR|O_CREAT);
	if (fd == -1) {
		fprintf(stderr, "Cant open file %s\n",
			filename);
		return 0;
	}

	size = write(fd, buf, size);
	if (size == -1) {
		fprintf(stderr, "Cant write to file; errno: %d\n",
		    errno);
	}
	
	close(fd);
	return 0;
}

int
main(int argc, char **argv)
{
	struct pkg *pkg = NULL;
	const char *file = NULL;
	char *basefile = NULL;
	char targetfilename[256];
	int fd;
	struct pkg_manifest_key *keys = NULL;
	DIR	*dirp;

	if (argc != 3) {
		usage_info(argv[0]);
		return (EX_USAGE);
	}

	/* Test if the directory exists */
	dirp = opendir(argv[2]);
	if (dirp == NULL) {
		fprintf(stderr, "%s is not a directory\n",
		    argv[2]);
		usage_info(argv[0]);
		return (EX_USAGE);
	}
	closedir(dirp);
	
	/* Get filename */
	file = argv[1];
	basefile = basename(argv[1]);
	if (file == NULL || basefile == NULL) {
		usage_info(argv[0]);
		return (EX_USAGE);
	}

	/* Open file */
	if ((fd = open(file, O_RDONLY)) == -1) {
		fprintf(stderr, "Unable to open %s\n",
			file);
		return (EX_IOERR);
	}

	/* Allocate internal structures for keys and open pkg */
	pkg_manifest_keys_new(&keys);
	if (pkg_open_fd(&pkg, fd, keys, 0) != EPKG_OK) {
		fprintf(stderr, "pkg_open_fd failed\n");
		close(fd);
		return (EX_IOERR);
	}
	/* Close file and free keys */
	close(fd);
	pkg_manifest_keys_free(keys);

	/* Extract PRE/POST/INSTALL script and write to file */
	snprintf(targetfilename, 256, "%s/pre_install_%s.sh",
	    argv[2], basefile);
        write_pkg_script(pkg, PKG_SCRIPT_PRE_INSTALL, targetfilename);

	snprintf(targetfilename, 256, "%s/install_%s.sh",
	    argv[2], basefile);
        write_pkg_script(pkg, PKG_SCRIPT_INSTALL, targetfilename);

	snprintf(targetfilename, 256, "%s/post_install_%s.sh",
	    argv[2], basefile);
        write_pkg_script(pkg, PKG_SCRIPT_POST_INSTALL, targetfilename);

	/* Free pkg */
	pkg_free(pkg);
	return (EX_OK);
}
