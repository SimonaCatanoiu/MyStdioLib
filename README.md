# Tema 1 - Biblioteca stdio
## Overview
<p align="justify">
Acest proiect este o tema de laborator pentru cursul de Proiectarea sistemelor de operare si reprezinta o implementare minmala a bibliotecii stdio, care sa permita lucrul cu fisiere. Biblioteca implementeaza structura SO_FILE, impreuna cu functiile de citire, scriere, oferind si functionalitatea de buffering.
</p>

## Functiile implementate
* **so_fopen:** deschide un fisier in modul precizat si initializeaza o structura SO_FILE corespunzatoare. Foloseste apelul de sistem ```open``` pentru a realiza acest lucru.
* **so_fclose:** dezaloca structura SO_FILE si inchide fisierul deschis, folosind apelul de sistem ```close```. De asemenea, inainte de a inchide fisierul, pentru a ne asigura ca nu au ramas date nescrise in fisier din buffer, apelam un ```so_fflush```.
* **so_fileno:** intoarce descriptorul fisierului asociat structurii SO_FILE.
* **so_fflush:** este functia care asigura scrierea in fisier a datelor din buffer prin utilizarea apelului de sistem ```write```. Am folosit un while pentru ca apelul de sistem nu asigura scrierea tuturor octetilor in fisier si acest lucru era necesar. De asemenea, functia invalideaza bufferul dupa scriere.
* **so_fgetc:** citeste un caracter din fisier si se bazeaza pe buffering (se ocupa cu popularea bufferului). Functia foloseste apelul de sistem ```read``` pentru a citi un bloc de octeti pe care ii incarca in buffer atunci cand acesta este gol sau bufferul precedent a fost citit complet. Caracterul intors este citit direct din buffer pe baza unui offset care se incrementeaza la fiecare apel al functiei. 
* **so_fputc:** scrie un caracter in fisier si se bazeaza pe buffering.(se ocupa cu popularea bufferului) .Functia foloseste ```so_fflush``` pentru a scrie continutul bufferului atunci cand acesta a fost umplut. Caracterul este scris direct in buffer pe baza unui offset care se incrementeaza la fiecare apel al functiei. 
* **so_fread:** citeste un bloc de elemente de o anumita dimensiune si le salveaza la o adresa specificata de un pointer. Pentru a realiza citirea, se foloseste de functia ```so_fgetc```, care este apelata pentru citirea fiecarui octet in parte in bufferul structurii SO_FILE.
* **so_fwrite:** scrie un bloc de elemente de o anumita dimensiune dintr-un buffer primit ca parametru. Pentru a asigura scrierea, se foloseste de functia ```so_fputc```, care este apelata pentru a scrie fiecare octet in parte in bufferul structurii SO_FILE.
* **so_fseek:** muta cursorul fisierului la o pozitie dorita folosind apelul de sistem ```lseek```. De asemenea, se ocupa cu invalidarea bufferului in cazul in care ultima operatie a fost de citire si apeleaza ```so_fflush``` in cazul in care ultima operatie a fost de scriere, pentru a asigura scrierea datelor existente in buffer in fisier.
* **so_ftell:** intoarce pozitia curenta a cursorului din fisier. Acest lucru este realizat prin variabila ```file_cursor``` din SO_FILE care se incrementeaza la fiecare apel so_fputc sau so_sgetc. De asemenea, valoarea este modificata in cazul unui apel ```so_fseek```.
* **so_feof:** inotarce o valoare diferita de 0 daca s-a ajuns la EOF
* **so_ferror:** intoarce o valoare diferita de 0 daca a aparut o eroare in urma executiei functiilor din librarie.
* **so_popen:** lanseaza un proces nou folosind ```fork``` care sa execute o anumita comanda. Se foloseste de un pipe anonim pentru a realiza comunicarea intre procesul parinte (programul) si copil (bash). Fiecare proces asculta sau scrie la unul din capete si il inchide pe cel nefolosit. In cazul copilului, acesta redirecteaza standard input/output in capatul corespunzator si executa comanda care i-a fost data folosind apelul de sistem ```execvp```. Parintele aloca o structura SO_FILE, seteaza pid-ul copilului in aceasta si o returneaza.
* **so_pclose:** asteapta terminarea procesului lansat de ```so_popen``` si elibereaza memoria ocupata de structura ```SO_FILE```.

## Structura SO_FILE

* **handle:** descriptorul fisierului deschis
* **flags:** flagurile folosite pentru a descrie comporatamentul fisierul deschis (O_RONLY etc.)
* **filemode:** este setat cu valoarea implicita de 0644 doar daca fisierul este deschis intr-un mod ce presupune si crearea acestuia. Altfel, are valoarea -1
* **buffer:** este bufferul de dimensiune ```BUFF_SIZE``` folosit pentru scrierea/citirea din fisier
* **buffer_offset:** reprezinta cursorul asociat bufferului. El se modifica odata ce citim/scriem
* **buffer_length:** reprezinta dimensiunea ocupata a bufferului. (nu mereu se citesc/scriu ```BUFF_SIZE``` octeti)
* **file_offset:** reprezinta cursorul fisierului
* **bool_is_eof:**  ```true```/```false``` daca s-a ajuns la EOF
* **bool_is_error:** ```true```/```false``` daca a avut loc o eroare
* **pid:** pid-ul procesului copil. Ne este necesar pentru a stii pe cine astepta parintele in functia ```pclose```
* **last_operation:** tine minte daca ultima operatie folosita a fost de scriere/citire. Am folosit un enum pentru a defini aceste stari
* **openmode:** tine minte in ce mod a fost deschis fisierul. Am folosit un enum pentru a defini aceste stari

## Bibliografie:
* Pentru implementarea ```so_popen```/```so_pclose``` m-am ajutat de: https://android.googlesource.com/platform/bionic/+/3884bfe9661955543ce203c60f9225bbdf33f6bb/libc/unistd/popen.c
* Man page-urile apelurilor de sistem folosite
* Pentru intelegere buffering: https://stackoverflow.com/questions/29712445/what-is-the-use-of-buffering-in-pythons-built-in-open-function,https://stackoverflow.com/questions/27993971/understanding-buffering-in-c
