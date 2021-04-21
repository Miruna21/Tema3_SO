Nume: Orzata Miruna-Narcisa,
Grupă: 331CA

# Tema 3 SO

Organizare
-
* Ideea: Implementarea unui **loader de fisiere executabile statice** sub forma unei biblioteci partajate.
* Solutia se bazeaza pe dezvoltarea unui program in C pe sistemele **Unix si Windows**, care incarca un fișier executabil în memorie pagină cu pagină, folosind mecanismul **demand paging**.


Implementare
-
* Primul pas consta in parsarea fisierului binar (**ELF - Linux, PE - Windows**), in urma careia se populeaza structura so_exec, ce cuprinde informatii utile despre fiecare segment din executabil si date generale despre executabilul propriu-zis.
* Apoi se va executa prima instructiune din executabil (entry point-ul).
* Deoarece nu am incarcat nimic din executabil in memorie, de-a lungul execuției, se va genera câte un page fault pentru fiecare acces la o pagină nemapată în memorie. In aceste sens, am implementat o rutina de tratare a semnalului **SIGSEGV** prin care pot sa verific daca accesul la o anumita zona de memorie s-a produs in urma unei actiuni nepermise (accesam o zona ce nu apartine niciunui segment din executabil sau nu avem permisiunile necesare asupra zonei respective) sau adresa se gaseste intr-un anumit segment, dar pagina care o contine nu este mapata in memorie. In cazul in care pagina nu este mapata in memorie, va trebui sa citesc datele pentru pagina respectiva din fisier, sa mapez pagina in memorie si sa copiez datele obtinute in acea zona de memorie.
    
* Alte detalii:
    * Daca file_size < mem_size pentru un segment si ce s-a citit din fisier nu a ajuns la dimensiunea unei pagini, atunci restul de spatiu din pagina se va umple cu 0 pana cand se umple toata pagina sau pana la mem_size.
    * Maparea memoriei virtuale s-a realizat intr-o zona fixa de memorie cu ajutorul functiilor **mmap / VirtualAlloc**, avand permisiuni de scriere pentru a se putea copia datele obtinute din executabil in acea zona. Ulterior, au fost schimbate permisiunile paginii mapate cu permisiunile segmentului din care face parte prin intermediul functiilor **mprotect / VirtualProtect**.
    * Interceptarea semnalul SIGSEGV si tratarea acestuia printr-o rutina s-a bazat pe folosirea apelurilor **sigaction pe Linux** si pe inregistrarea unui handler in **vectorul de exceptii pe Windows**. 



Cum se compilează și cum se rulează?
-
* In urma comenzii make / nmake, se genereaza biblioteca dinamica libso_loader.so / libso_loader.dll.
* Comanda "make -f Makefile.checker" / "nmake /F NMakefile.checker" va compila sursele folosite de checker.
* Fiecare test se ruleaza separat astfel: "_test/run_test.sh init", urmat de "_test/run_test.sh <nr_test>"
* Toate testele se ruleaza prin: "./run_all.sh".

Resurse utilizate
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