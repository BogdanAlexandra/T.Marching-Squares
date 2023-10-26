// Author: APD team, except where source was noted

#include "helpers.h"
#include "paralelizare.h"
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


// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
ppm_image **init_contour_map() {
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        map[i] = read_ppm(filename);
    }

    return map;
}

void *thread_function(void *arg){

    thread_data_s *data = (thread_data_s *) arg;

    int P = data->num_threads;

    //rescale image 
	rescale_image(P, data);

    // Wait for all threads to finish rescaling
    pthread_barrier_wait(data->barrier);

    //sample the grid
    sample_grid(P, data);

    // Wait for all threads to finish calculating the grid
    pthread_barrier_wait(data->barrier);
    
    // Marching the squares
    march(data, P);
    
    pthread_exit(NULL);

}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}

//alocate memory for the grid
unsigned char **allocate_memory_for_grid(int p, int q)
{

    unsigned char **grid = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));
    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i <= p; i++) {
        grid[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
        if (!grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
    }

    return grid;
}

// allocate memory for image 
ppm_image *allocate_memory_for_rescale_image(ppm_image *image){

     if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
        return image;
      }
    
    ppm_image *new_image = (ppm_image *)malloc(sizeof(ppm_image));
    if (!new_image) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    new_image->x = RESCALE_X;
    new_image->y = RESCALE_Y;

    new_image->data = (ppm_pixel*)malloc(new_image->x * new_image->y * sizeof(ppm_pixel));
    if (!new_image) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    return new_image;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    pthread_barrier_t barrier;
    int num_threads = atoi(argv[3]);
    pthread_t threads[num_threads];
    thread_data_s thread_info[num_threads];
    int i;

    pthread_barrier_init(&barrier, NULL, num_threads);
   
    ppm_image *image = read_ppm(argv[1]);
    int step_x = STEP;
    int step_y = STEP;

    // 0. Initialize contour map
    ppm_image **contour_map = init_contour_map();

    //alocate memory for image
    ppm_image *scaled_image = allocate_memory_for_rescale_image(image);

    int p = scaled_image->x / step_x;
    int q = scaled_image->y / step_y;

    //alocate memory for the grid
    unsigned char **grid = allocate_memory_for_grid(p,q);

    //create threads
    for (i = 0; i < num_threads; i++) {
        thread_info[i].thread_id = i;
        thread_info[i].step_x = step_x;
        thread_info[i].step_y = step_y;
        thread_info[i].num_threads = num_threads;
        thread_info[i].grid = grid;
        thread_info[i].sigma = SIGMA;
        thread_info[i].contour_map = contour_map;
        thread_info[i].original_image = image;
        thread_info[i].new_image = scaled_image;
        thread_info[i].barrier = &barrier;

        int rc = pthread_create(&threads[i], NULL, thread_function, (void *)&thread_info[i]);
        if (rc) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
    }

    for (i = 0; i < num_threads; i++) {
         int r_j = pthread_join(threads[i], NULL);
         if (r_j) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}

    }

    //destroy barrier
    pthread_barrier_destroy(&barrier);

    // 4. Write output
    write_ppm(scaled_image, argv[2]);

    free_resources(scaled_image, contour_map, grid, step_x);

    return 0;
}
