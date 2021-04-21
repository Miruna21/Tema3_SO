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
#include <math.h>
#include <sys/types.h>

#include <Windows.h>

#define DLL_EXPORTS
#include "loader.h"
#include "exec_parser.h"


#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

static so_exec_t *exec;
char *file_path;
static int page_size = 0x10000;

struct seg_extra_info {
	int nr_pages;
	int *mapped;
};

int so_init_loader(void)
{
	/* TODO: initialize on-demand loader */
	return -1;
}

int so_execute(char *path, char *argv[])
{
	struct seg_extra_info extra_info;
	int i, nr_pages;

	exec = so_parse_exec(path);

	if (!exec)
		return -1;

	file_path = malloc(30 * sizeof(char));
	memcpy(file_path, path, 30);

	for (i = 0; i < exec->segments_no; i++) {
		/* retin paginile mapate sau nemapate din fiecare segment */
		memset((void *)&extra_info, 0, sizeof(struct seg_extra_info));
		nr_pages = (int)max(1,
			ceil(exec->segments[i].mem_size / (float)page_size));

		extra_info.nr_pages = nr_pages;
		extra_info.mapped = malloc(nr_pages * sizeof(int));
		/* la inceput, paginile sunt nemapate in memorie */
		memset(extra_info.mapped, 0, nr_pages * sizeof(int));
		exec->segments[i].data = malloc(sizeof(struct seg_extra_info));
		memcpy(exec->segments[i].data, &extra_info,
						sizeof(struct seg_extra_info));
	}

	so_start_exec(exec, argv);

	return 0;
}
