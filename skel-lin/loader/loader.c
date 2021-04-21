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


static int page_size;
static struct sigaction old_action;

void get_data_from_file(unsigned int start_offset, unsigned int size, unsigned char *data)
{
	/* citesc din fisier datele dintr-o anumita portiune a unui segment */
	int fd;
	int ret;

	fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		perror("open error\n");
		exit(EXIT_FAILURE);
	}

	off_t pos = lseek(fd, start_offset, SEEK_SET);

	if (pos < 0) {
		perror("lseek error\n");
		exit(EXIT_FAILURE);
	}

	int crt_bytes_read = read(fd, data, size);

	if (crt_bytes_read < 0 || crt_bytes_read == 0) {
		perror("read error\n");
		exit(EXIT_FAILURE);
	}

	ret = close(fd);

	if (ret < 0) {
		perror("close error\n");
		exit(EXIT_FAILURE);
	}
}

static void segfault_handler(int signum, siginfo_t *info, void *context)
{
	char *addr = (char *)info->si_addr;
	int found = 0;
	int ret;

	if (signum != SIGSEGV) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}


	for (int i = 0; i < exec->segments_no; i++) {
		so_seg_t this_seg = exec->segments[i];
		uintptr_t seg_vaddr = (uintptr_t)this_seg.vaddr;
		unsigned int seg_size = this_seg.mem_size;

		if ((uintptr_t)addr >= seg_vaddr && (uintptr_t)addr <= (seg_vaddr + seg_size)) {
			/* adresa cauzatoare se gaseste intr-un segment din executabil */
			found = 1;
			int page = ceil((uintptr_t)((uintptr_t)addr - (uintptr_t)seg_vaddr) / page_size);
			struct seg_extra_info *extra_info = (struct seg_extra_info *)this_seg.data;

			if (extra_info->mapped[page] == 0) {
				/* pagina nu este mapata in memorie */
				unsigned int page_data_size;
				unsigned int page_start_offset_file = this_seg.offset + page * page_size;

				if (page * page_size > this_seg.file_size) {
					/* nu a mai ramas nimic de citit din fisier */
					page_data_size = 0;
				} else {
					/* determin cat trebuie sa citesc din fisier */
					page_data_size = min(page_size, page_size - ((page + 1) * page_size - this_seg.file_size));
				}

				unsigned char *data = malloc(page_size * sizeof(char));

				if (page_data_size != 0)
					get_data_from_file(page_start_offset_file, page_data_size, data);

				if (page_data_size < page_size) {
					/* umplu restul paginii cu 0 */
					memset((void *)(data + page_data_size), 0, page_size - page_data_size);
				}

				void *page_start_vaddr = (void *)seg_vaddr + page * page_size;

				/* mapez pagina in memorie cu permisiuni de scriere */
				char *mapped_mem = mmap((void *)page_start_vaddr, page_size, PROT_WRITE,
										MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

				if (mapped_mem == MAP_FAILED) {
					perror("mmap error\n");
					exit(EXIT_FAILURE);
				}

				/* se scriu datele in zona de memorie anterior mapata */
				memcpy(mapped_mem, data, page_size);
				/* schimb permisiunile initiale ale paginii cu permisiunile segmentului */
				ret = mprotect(mapped_mem, page_size, this_seg.perm);
				if (ret == -1) {
					perror("mprotect error");
					exit(EXIT_FAILURE);
				}

				free(data);
				extra_info->mapped[page] = 1;
			} else {
				/* pagina e deja mapata => nu avem permisiunile necesare */
				old_action.sa_sigaction(signum, info, context);
				return;
			}

			break;
		}
	}

	if (found == 0) {
		/* adresa nu se gaseste in niciun segment din executabil */
		old_action.sa_sigaction(signum, info, context);
		return;
	}
}


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

