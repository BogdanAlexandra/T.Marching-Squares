Algoritmi Paraleli si Distribuiti---Tema-1
====================

> Student: Bogdan Alexandra-Lacramioara
> Grupa: 334CD
=====================

* Biblioteci folosite 
 > #include <pthread.h>

* Elemente de sinconizare
 > bariera

* Descriere fisiere
-> helpers.c / helpers.h
-> paralelizare.c / paralelizare.h  = aici am implementat functiile paralelizate rescale_image, sample_grid si march
-> tema1_par.c = main
-> Makefile
-> README 


*** Detalii implementare ***

Pentru fiecare thread am folosit propria sa structura de date, respectiv thread_data_s regasita in fisierul paralelizare.h. Aceasta structura ii este construita fix inainte sa fie creat threadul, folosind functia pthread_create.

* Structura de date 

 -> regasita in fisierul paralelizare.h 

    typedef struct {
        int thread_id; //identificatorul unic al fiecarui thread
        int num_threads; //numărul total de fire de execuție
        int step_x; // pasii pe axa X
        int step_y; // pasii pe axa y
        unsigned char sigma; // valoare numerica 
        unsigned char **grid; // matrice bidimensională de tip unsigned char care este   utilizată pentru a stoca informații legate de gridul imaginii.
        ppm_image *new_image; // imaginea rescalata
        ppm_image *original_image; //imaginea originala 
        ppm_image **contour_map;
        pthread_barrier_t *barrier; //barieră de sincronizare 
    } thread_data_s;


Pentru paralizarea algoritmului Marching Squares am ales sa paralizez cele trei functii: rescale_image, sample_grid si march, acest lucru realizandu-l in fisierul paralelizare.c / paralelizare.h.


** Impartirea pe threaduri ** 

Am facut impartirea pe threaduri dupa formula din cadrul laboratorului 2:

> int start = thread_id * (double) N / P;

> int end = min((thread_id + 1) * (double) N / P, N);


** Impartirea pe threaduri pentru fiecare functie **

* Pentru ** rescale_image **:

 int start_r = data->thread_id * (double)data->new_image->x / data->num_threads

 int end_r = mymin((data->thread_id + 1) * (double)data->new_image->x / data->num_threads, data->new_image->x)

* Pentru ** sample_grid **

1. pentru primele doua bucle for paralelizate
int start_g = data->thread_id * (double) p / data->num_threads
int end_g = mymin((data->thread_id + 1) * (double) p / data->num_threads, p) 
     
    ,unde 
    p = data->new_image->x / data->step_x
    q = data->new_image->y / data->step_y

2. pentru a treia bucla for paralelizata
int start_q = data->thread_id * (double)q / data->num_threads;
int end_q = mymin((data->thread_id + 1) * (double)q / data->num_threads, q);
    
* Pentru ** march **

int start_m = data->thread_id * (double)p_m / data->num_threads
int end_m = mymin((data->thread_id + 1) * (double)p_m / data->num_threads, p_m)

    ,unde 
    p_m = data->new_image->x / data->step_x
    q_m = data->new_image->y / data->step_y
    

** Paralelizarea **

Pentru functia rescale_image am ales sa paralelizez primul for din varianta seriala, restul ramanand neschimbat. Imediat dupa am folosit o bariera ca sa sincronisez toate threadurile.

Pentru sample_grid există trei bucle for care au fost paralelizate, deoarece fiecare buclă este încapsulată într-un bloc de cod care este executat în paralel de către mai multe fire de execuție. În fiecare buclă, valorile sunt calculate pentru fiecare celulă a gridului, pe baza culorii medii a pixelilor din imaginea rescalată. Aceste valori sunt apoi stocate în matricea data->grid. Dupa fiecare set de instructiuni corespunzator buclei for paralelizate am folosit o bariera pentru sincronziarea tuturor threadurilor.

Pentru march am paralelizat prima bucla for dupa formula prezentata mai sus.În fiecare iterație a acestei bucle, se calculează o valoare k pe baza configurației celulelor din gridul definit de data->grid. Această valoare k este apoi folosită pentru a determina cum trebuie tras conturul în imaginea data->new_image. Funcția update_image este apelată pentru a realiza această actualizare a imaginii de contur.


** Functia thread_function **

Functia thread_function este folosită pentru a efectua operatii paralele asupra datelor, in ea apeland functiile rescale_image, sample_grid, respectiv  march. Thread-ul asteapta la bariere intre aceste etape pentru a coordona operatiile intre mai multe thread-uri. 


** Alocarea de memorie **

Alocarea de memorie pentru grid si rescale_image se realizeaza in functiile auxiliare allocate_memory_for_grid, respectiv allocate_memory_for_rescale_image. Acest lucru este realizat la fel ca in varianta seriala.


** Rulare **

-> make : compileaza sursele
-> make clean : sterge tema1, tema1_par 

** Feedback **

Tema a fost super ok. Daca erai atent la laboratorul 2, tema se rezolva relativ repede. :)

