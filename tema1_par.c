/*
 * APD - Project 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

pthread_barrier_t barrier_large;
pthread_barrier_t barrier_small;

int P;
int **result;

char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;

// structure for a complex number
typedef struct _complex {
	double a;
	double b;
} complex;

// structure for the parameters 
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

//structure to send the necessary parameters to the thread function
typedef struct _thread_struct {
	int tid;
	params *general_par; 
} thread_struct;

// read the programm arguments
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// read the input file
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// write output in the output file
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// alloc memory for the result
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// free the memory
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

// run Julia algorithm
void run_julia(params *par, int **result, int width, int height, int thread_id)
{
	int w, h, i;

	int start1 = thread_id * (double) height / P;
	int end1 = MIN((thread_id + 1) * (double) height / P, height);

	for (h = start1; h < end1; h++) {
		for (w = 0; w < width; w++) {
			int step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier_small);

	// transform the result from mathematics coordinates to display coordinates
	int start2 = thread_id * (double) (height / 2) / P;
	int end2 = MIN((thread_id + 1) * (double) (height / 2) / P, height);

	for (i = start2; i < end2; i++) {
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
}

// run Mandelbrot algorithm
void run_mandelbrot(params *par, int **result, int width, int height, int thread_id)
{
	int w, h, i;
	int start1 = thread_id * (double) height / P;
	int end1 = MIN((thread_id + 1) * (double) height / P, height);

	for (h = start1; h < end1; h++) {
		for (w = 0; w < width; w++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier_small);
	
	// transform the result from mathematics coordinates to display coordinates

	int start2 = thread_id * (double) (height / 2) / P;
	int end2 = MIN((thread_id + 1) * (double) (height / 2) / P, height);
	for (i = start2; i < end2; i++) {
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
}

void *thread_function(void *arg) {

	thread_struct data = *(thread_struct *) arg;

	int thread_id = data.tid;
	params *par_files = data.general_par;

	// calculus of the parameters for Julia function
	int width_julia = (par_files->x_max - par_files->x_min) / par_files->resolution;
	int height_julia = (par_files->y_max - par_files->y_min) / par_files->resolution;

	run_julia(par_files, result, width_julia, height_julia, thread_id);
	
	// main synchronization
	pthread_barrier_wait(&barrier_large);
	// here the main reads Mandelbort's input and writes Julia's output
	pthread_barrier_wait(&barrier_large);

	// calculus of the parameters for Mandelbrot function
	int width_mandelbrot = (par_files->x_max - par_files->x_min) / par_files->resolution;
	int height_mandelbrot = (par_files->y_max - par_files->y_min) / par_files->resolution;

	run_mandelbrot(par_files, result, width_mandelbrot, height_mandelbrot, thread_id);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	params par;
	int width, height;
	void *status;

	get_args(argc, argv);

	pthread_t tid[P];
	thread_struct threads[P];
	
	pthread_barrier_init(&barrier_large, NULL, P + 1);
	pthread_barrier_init(&barrier_small, NULL, P);
	
	read_input_file(in_filename_julia, &par);
	width = (par.x_max - par.x_min) / par.resolution;
	height = (par.y_max - par.y_min) / par.resolution;

	result = allocate_memory(width, height);

	for (int i = 0; i < P; i++) {

		 threads[i].tid = i;
		 threads[i].general_par = &par;
		 pthread_create(&tid[i], NULL, thread_function, &(threads[i]));

	}

	// waits for Julia processing
	pthread_barrier_wait(&barrier_large); 

	write_output_file(out_filename_julia, result, width, height);
	free_memory(result, height);
	
	// Mandelbrot:

	read_input_file(in_filename_mandelbrot, &par);
	width = (par.x_max - par.x_min) / par.resolution;
	height = (par.y_max - par.y_min) / par.resolution;
	result = allocate_memory(width, height);

	// release the threads to process Mandelbrot
	pthread_barrier_wait(&barrier_large);

	for(int i = 0; i < P; i++) {
		pthread_join(tid[i], &status);
	}

	write_output_file(out_filename_mandelbrot, result, width, height);
	free_memory(result, height);

	pthread_barrier_destroy(&barrier_large);
	pthread_barrier_destroy(&barrier_small);

	return 0;
}