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


int mymin(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

void rescale_image(int P, thread_data_s *data)
{
	uint8_t sample[3];

    // we only rescale downwards
    if (data->original_image->x <= RESCALE_X && data->original_image->y <= RESCALE_Y) {
        data->new_image = data->original_image;
    }

    else {

        int start_r = data->thread_id * (double)data->new_image->x / P;
        int end_r = mymin((data->thread_id + 1) * (double)data->new_image->x / P, data->new_image->x);

    // use bicubic interpolation for scaling
     for (int i = start_r; i < end_r; i++) {
        for (int j = 0; j < data->new_image->y; j++) {
            float u = (float)i / (float)(data->new_image->x - 1);
            float v = (float)j / (float)(data->new_image->y - 1);
            sample_bicubic(data->original_image, u, v, sample);

            data->new_image->data[i * data->new_image->y + j].red = sample[0];
            data->new_image->data[i * data->new_image->y + j].green = sample[1];
            data->new_image->data[i * data->new_image->y + j].blue = sample[2];
        }
    }
}
}

void sample_grid(int P, thread_data_s *data) {
     int p = data->new_image->x / data->step_x;
     int q = data->new_image->y / data->step_y;

     int start_g = data->thread_id * (double) p / P;
     int end_g = mymin((data->thread_id + 1) * (double) p / P, p);
  
    for (int i = start_g; i < end_g; i++) {
      for (int j = 0; j < q; j++) {
            ppm_pixel curr_pixel = data->new_image->data[i * data->step_x * data->new_image->y + j * data->step_y];
            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > data->sigma) {
                data->grid[i][j] = 0;
            } else {
                data->grid[i][j] = 1;
            }
        }
    }


    pthread_barrier_wait(data->barrier);


    for (int i = start_g; i < end_g; i++) {
        ppm_pixel curr_pixel = data->new_image->data[i * data->step_x * data->new_image->y + data->new_image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;
        if (curr_color > data->sigma) {
            data->grid[i][q] = 0;
        } else {
            data->grid[i][q] = 1;
        }
    }

    pthread_barrier_wait(data->barrier);

    int start_q = data->thread_id * (double)q / P;
    int end_q = mymin((data->thread_id + 1) * (double)q / P, q);

    for (int j = start_q; j < end_q; j++) {
        ppm_pixel curr_pixel = data->new_image->data[(data->new_image->x - 1) * data->new_image->y + j * data->step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > data->sigma) {
            data->grid[p][j] = 0;
        } else {
            data->grid[p][j] = 1;
        }
    }
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {
    for (int i = 0; i < contour->x; i++) {
        for (int j = 0; j < contour->y; j++) {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}

void march(thread_data_s *data, int P){

    int p_m = data->new_image->x / data->step_x;
    int q_m = data->new_image->y / data->step_y;

    int start_m = data->thread_id * (double)p_m / P;
    int end_m = mymin((data->thread_id + 1) * (double)p_m / P, p_m);


    for (int i = start_m; i < end_m; i++) {
        for (int j = 0; j < q_m; j++) {
            unsigned char k = 8 * data->grid[i][j] + 4 * data->grid[i][j + 1] + 2 * data->grid[i + 1][j + 1] + 1 * data->grid[i + 1][j];
            update_image(data->new_image, data->contour_map[k], i * data->step_x, j * data->step_y);
        }
    }
}