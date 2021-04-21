/*
 * Loader Implementation
 *
 * 2021, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

#include "exec_parser.h"


#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

static so_exec_t *exec;
char *file_path;

struct seg_extra_info {
	int nr_pages;
	int *mapped;
};


/* setez un handler pentru tratarea semnalului SIGSEGV */
static void signal_handler_configuration(void)
{
	struct sigaction action;
	int ret;

	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = segfault_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);

	ret = sigaction(SIGSEGV, &action, &old_action);
	if (ret == -1) {
		perror("sigaction error\n");
		exit(EXIT_FAILURE);
	}
}

int so_init_loader(void)
{
	page_size = getpagesize();
	signal_handler_configuration();

	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);

	if (!exec)
		return -1;

	file_path = malloc(30 * sizeof(char));
	memcpy(file_path, path, 30);

	for (int i = 0; i < exec->segments_no; i++) {
		/* retin paginile mapate sau nemapate din fiecare segment */
		struct seg_extra_info extra_info;
		int nr_pages = ceil(exec->segments[i].mem_size / page_size);

		extra_info.nr_pages = nr_pages;
		extra_info.mapped = malloc(nr_pages * sizeof(int));
		/* la inceput, paginile sunt nemapate in memorie */
		memset(extra_info.mapped, 0, nr_pages * sizeof(int));
		exec->segments[i].data = malloc(sizeof(struct seg_extra_info));
		memcpy(exec->segments[i].data, &extra_info, sizeof(struct seg_extra_info));
	}

	so_start_exec(exec, argv);

	return 0;
}
