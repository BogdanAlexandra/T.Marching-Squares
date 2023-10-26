
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

// structure for each thread data 
typedef struct {
    int thread_id;
    int num_threads;
    int step_x;
    int step_y;
    unsigned char sigma;
    unsigned char **grid;
    ppm_image *new_image;
    ppm_image *original_image;
    ppm_image **contour_map;
    pthread_barrier_t *barrier;
} thread_data_s;

void rescale_image(int P, thread_data_s *data);
void sample_grid(int P, thread_data_s *data);
void update_image(ppm_image *image, ppm_image *contour, int x, int y) ;
void march(thread_data_s *data, int P);
