Name: Orzata Miruna-Narcisa,
Group: 331CA

# Project 3 SO

Organization
-
* Idea: Implement a **static executable file loader** in the form of a shared library.
* The solution is based on the development of a C program on **Unix and Windows** systems, which loads an executable file in page-by-page memory, using the **demand paging** mechanism.


Implementation
-
* The first step is to pars the binary file (**ELF - Linux, PE - Windows**), which populates the so_exec structure, which contains useful information about each segment of the executable and general data about the executable itself.
* Then the first instruction in the executable (entry point) will be executed.
* Because we have not loaded anything from the executable into memory, during execution, a page fault will be generated for each access to an unmapped page in memory. In this sense, we have implemented a signal processing routine **SIGSEGV** through which I can check if the access to a certain memory area occurred as a result of an illegal action (accessing an area that does not belong to any segment of the executable or not we have the necessary permissions on that area) or the address is in a certain segment, but the page that contains it is not mapped to memory. If the page is not mapped to memory, I will need to read the data for that page from the file, map the page to memory, and copy the data obtained to that memory area.
    
* Other details:
    * If file_size <mem_size for a segment and what was read from the file did not reach the size of a page, then the remaining space on the page will be filled with 0 until the entire page is filled or until mem_size.
    * The virtual memory mapping was done in a fixed area of ​​memory using the **mmap / VirtualAlloc** functions, with write permissions to be able to copy the data obtained from the executable in that area. Subsequently, the permissions of the mapped page were changed with the permissions of the segment to which it belongs through the **mprotect / VirtualProtect** functions.
    * The interception of the SIGSEGV signal and its handling through a routine was based on the use of **sigaction calls on Linux** and on the registration of a handler in **the exception vector on Windows**.



How is it compiled and run?
-
* Following the make / nmake command, the dynamic library libso_loader.so / libso_loader.dll is generated.
* The "make -f Makefile.checker" / "nmake / F NMakefile.checker" command will compile the sources used by the checker.
* Each test is run separately as follows: "_test / run_test.sh init", followed by "_test / run_test.sh <nr_test>"
* All tests are run through: "./run_all.sh".

Resources used
-
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-05
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
* https://man7.org/linux/man-pages/man2/mmap.2.html
* https://man7.org/linux/man-pages/man2/mprotect.2.html
* https://man7.org/linux/man-pages/man2/sigaction.2.html
* https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler?redirectedfrom=MSDN
* https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc?redirectedfrom=MSDN
* https://docs.microsoft.com/en-us/previous-versions/ms913495(v=msdn.10)

Git
-
* https://github.com/Miruna21/Tema3_SO 
