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

static LPVOID action;


void get_data_from_file(unsigned int start_offset,
				unsigned int size, unsigned char *data)
{
	HANDLE hFile;
	DWORD pos;
	DWORD crt_bytes_read;
	BOOL ret;

	/* citesc din fisier datele dintr-o portiune a unui segment */
	hFile = CreateFile(file_path,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		perror("CreateFile error\n");
		exit(EXIT_FAILURE);
	}

	pos = SetFilePointer(hFile, start_offset, NULL, SEEK_SET);

	if (pos == INVALID_SET_FILE_POINTER
			&& GetLastError() != NO_ERROR) {
		perror("setFilePointer error\n");
		exit(EXIT_FAILURE);
	}

	ret = ReadFile(hFile, data, size, &crt_bytes_read, NULL);

	if (ret == FALSE) {
		perror("ReadFile error\n");
		exit(EXIT_FAILURE);
	}

	ret = CloseHandle(hFile);

	if (ret == FALSE) {
		perror("CloseHandle error\n");
		exit(EXIT_FAILURE);
	}
}


unsigned int find_perm(unsigned int perm)
{
	unsigned int new_perm;

	switch (perm) {
	case 1:
		new_perm = PAGE_READONLY;
		break;
	case 2:
		new_perm = PAGE_READWRITE;
		break;
	case 3:
		new_perm = PAGE_READWRITE;
		break;
	case 4:
		new_perm = PAGE_EXECUTE;
		break;
	case 5:
		new_perm = PAGE_EXECUTE_READ;
		break;
	case 6:
		new_perm = PAGE_EXECUTE_READWRITE;
		break;
	case 7:
		new_perm = PAGE_EXECUTE_READWRITE;
		break;
	default:
		break;
	}

	return new_perm;
}

static LONG CALLBACK segfault_handler(PEXCEPTION_POINTERS ExceptionInfo)
{
	LPBYTE addr;
	int page, i;
	BOOL ret;
	DWORD old;
	char *mapped_mem;
	unsigned char *data;
	void *page_start_vaddr;
	unsigned int seg_size, page_start_offset_file, page_data_size;
	struct seg_extra_info *extra_info;
	so_seg_t this_seg;
	uintptr_t seg_vaddr;

	addr = (LPBYTE)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

	for (i = 0; i < exec->segments_no; i++) {
		this_seg = exec->segments[i];
		seg_vaddr = (uintptr_t)this_seg.vaddr;
		seg_size = this_seg.mem_size;

		if ((uintptr_t)addr >= seg_vaddr
				&& (uintptr_t)addr <= (seg_vaddr + seg_size)) {
			/* adresa se gaseste intr-un segment din executabil */
			page = (int)floor((uintptr_t)((uintptr_t)addr
					- (uintptr_t)seg_vaddr) / page_size);
			extra_info = (struct seg_extra_info *)this_seg.data;

			if (extra_info->mapped[page] == 0) {
				/* pagina nu este mapata in memorie */
				page_start_offset_file = this_seg.offset
							+ page * page_size;

				if ((unsigned int)page * page_size
						> this_seg.file_size) {
					/* nu e nimic de citit din fisier */
					page_data_size = 0;
				} else {
					/* determin cat sa citesc din fisier */
					page_data_size =
					min((unsigned int)page_size,
					page_size - ((page + 1) * page_size
						- this_seg.file_size));
				}

				data = malloc(page_size * sizeof(char));

				if (page_data_size != 0)
					get_data_from_file(
						page_start_offset_file,
						page_data_size, data);

				if (page_data_size <
					(unsigned int)page_size) {
					/* umplu restul paginii cu 0 */
					memset((void *)(data + page_data_size),
						0, page_size - page_data_size);
				}

				page_start_vaddr = (unsigned char *)seg_vaddr
							+ page * page_size;

				/* mapez pagina cu permisiuni de scriere */
				mapped_mem = VirtualAlloc(
						(void *)page_start_vaddr,
						page_size,
						MEM_COMMIT | MEM_RESERVE,
						PAGE_READWRITE);

				if (mapped_mem == NULL) {
					perror("VirtualAlloc error\n");
					exit(EXIT_FAILURE);
				}

				/* se scriu datele in zona de memorie mapata */
				memcpy(mapped_mem, data, page_size);
				/* setez permisiunile paginii */
				ret = VirtualProtect(mapped_mem, page_size,
					find_perm(this_seg.perm), &old);

				if (ret == FALSE) {
					perror("VirtualProtect error");
					exit(EXIT_FAILURE);
				}

				free(data);
				extra_info->mapped[page] = 1;
				return EXCEPTION_CONTINUE_EXECUTION;
			} else if (extra_info->mapped[page] == 1) {
				/* pagina e deja mapata-> nu avem permisiuni */
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}
	}

	/* adresa nu se gaseste in niciun segment din executabil */
	return EXCEPTION_CONTINUE_SEARCH;
}

/* setez un handler pentru tratarea semnalului SIGSEGV */
static void signal_handler_configuration(void)
{
	action = AddVectoredExceptionHandler(1, segfault_handler);

	if (action == NULL) {
		perror("AddVectoredExceptionHandler error\n");
		exit(EXIT_FAILURE);
	}
}

int so_init_loader(void)
{
	signal_handler_configuration();

	return 0;
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
