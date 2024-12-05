#include <stdlib.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION // Necesario para la implementación
#include "stb_image.h"  
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"  
#include <string.h>



#define SIGMA 6.0  // Standard deviation for Gaussian

typedef struct {
    char filter_name[50]; //stores the name of the filter
    int filter;       // Stores the filter id
    float rgb_r;      // Red component for RGB filter
    float rgb_g;      // Green component for RGB filter
    float rgb_b;      // Blue component for RGB filter
    int blurr_grid;   // Blurr grid size
    int zone;
    int x;	      // zone blurring x coordinate
    int y;	      // zone blurring y coordinate
} FilterOptions;

// Function to parse command-line arguments
int arg_parser(int argc, char **argv, FilterOptions *options) {
    // Ensure mandatory arguments are present
    if (argc < 3) {
        fprintf(stderr, "Usage: %s imagename [filter options]\n", argv[0]);
        return -1;
    }

    // Default filter values
    options->filter = -1;  // No filter by default
    options->rgb_r = options->rgb_g = options->rgb_b = 0.0f;

    // Start parsing optional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-rgb") == 0) {
            // Ensure there are enough arguments for RGB values
            if (i + 3 >= argc) {
                fprintf(stderr, "Error: -rgb flag requires 3 numerical arguments (0-1).\n");
                return -1;
            }
	    strcpy(options->filter_name, "rgb");
            options->filter = 5;  // Special value for RGB filter
            options->rgb_r = atof(argv[++i]);
            options->rgb_g = atof(argv[++i]);
            options->rgb_b = atof(argv[++i]);

            // Validate RGB values
            if (options->rgb_r < 0 || options->rgb_r > 1 ||
                options->rgb_g < 0 || options->rgb_g > 1 ||
                options->rgb_b < 0 || options->rgb_b > 1) {
                fprintf(stderr, "Error: RGB values must be between 0 and 1.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "-blurr") == 0){
        	if (i + 1 >= argc) {
		        fprintf(stderr, "Error: -rgb flag requires 1 numerical arguments (number of pixels (odd) used for the blurring grid).\n");
		        return -1;
            	}
            	strcpy(options->filter_name, "blurr");
            	options->filter = 6;
            	options->blurr_grid = atoi(argv[++i]);
            	if(options->blurr_grid%2 ==0){
            		fprintf(stderr, "Error: Blurring grid value must be odd.\n");
                return -1;
            	}
        } else if (strcmp(argv[i], "-bw") == 0){
         strcpy(options->filter_name, "bw");
         options->filter = 0; 
        } else if (strcmp(argv[i], "-inverted") == 0){
         strcpy(options->filter_name, "inverted");
         options->filter = 1; 
        } else if (strcmp(argv[i], "-r") == 0){
         strcpy(options->filter_name, "r");
         options->filter = 2; 
        } else if (strcmp(argv[i], "-g") == 0){
         strcpy(options->filter_name, "g");
         options->filter = 3; 
        } else if (strcmp(argv[i], "-b") == 0){
         strcpy(options->filter_name, "b");
         options->filter = 4; 
        } else if (strcmp(argv[i], "-zoneblurr") == 0){
         strcpy(options->filter_name, "zoneblurr");
         options->filter = 7;
          if (i + 4 >= argc) {
                fprintf(stderr, "Error: -zoneblurr flag requires 3 numerical arguments (grid size, s coordinate, y coordinate).\n");
                return -1;
            }
            options->blurr_grid = atoi(argv[++i]);
            options->zone = atoi (argv[++i]);
            options->x = atoi(argv[++i]);
            options->y = atoi(argv[++i]);
            if(options->blurr_grid%2 ==0){
            		fprintf(stderr, "Error: Blurring grid value must be odd.\n");
                return -1;
            }

        } else if (strcmp(argv[i], "-edge") == 0){
            strcpy(options->filter_name, "edge");
            options->filter = 8; 
        } else if (strcmp(argv[i], "-unblurr") == 0){
            strcpy(options->filter_name, "unblurr");
            options->filter = 9; 
        } else {
            fprintf(stderr, "Error: Unknown flag or argument: %s\n", argv[i]);
            return -1;
        }
    }

    // Ensure at least one filter option was provided
    if (options->filter == -1) {
        fprintf(stderr, "Error: No filter option provided.\n");
        return -1;
    }

    return 0;
}

void process_image (FilterOptions *flags, char * image_name, unsigned char* image, unsigned char* image_w_filter, int width, int height, int channels ){

    switch (flags->filter){
        case 0: // blanco y negro
            for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
                    unsigned char red = image[pixel];
                    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    unsigned char  gray_value = (red + green + blue)/3;
                    image_w_filter[pixel] = gray_value;
                    image_w_filter[pixel + 1] = gray_value; 
                    image_w_filter[pixel + 2] = gray_value;  
                }
            }
        break;
        case 1: //invertido
            for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
                    unsigned char red = image[pixel];
                    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    image_w_filter[pixel] = 255-red;
                    image_w_filter[pixel + 1] = 255-green; 
                    image_w_filter[pixel + 2] = 255-blue;  
                }
            }
        break;
        case 3: // no green
            for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
         	    unsigned char red = image[pixel];
         	    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    image_w_filter[pixel] = red;
                    image_w_filter[pixel + 1] = 0;
                    image_w_filter[pixel + 2] = blue;  
                }
            }
        break;
        case 4: // no blue
            for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
         	    unsigned char red = image[pixel];
         	    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    image_w_filter[pixel] = red;
                    image_w_filter[pixel + 1] = green;
                    image_w_filter[pixel + 2] = 0;  
                }
            }
       break;
       case 2: // no red
            for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
         	    unsigned char red = image[pixel];
         	    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    image_w_filter[pixel] = 0;
                    image_w_filter[pixel + 1] = green;
                    image_w_filter[pixel + 2] = blue;  
                }
            }
       break;
       case 5:
       	    for (int y = 0; y< height; y++) {
                for (int x = 0; x < width; x++) {
                    int pixel = 3*(y*width + x);
         	    unsigned char red = image[pixel];
         	    unsigned char green = image[pixel+1];
                    unsigned char blue = image[pixel+2];
                    image_w_filter[pixel] = red*flags->rgb_r;
                    image_w_filter[pixel + 1] = green*flags->rgb_g;
                    image_w_filter[pixel + 2] = blue*flags->rgb_b;  
                }
            }
       break;  
       case 6:
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel = 3 * (y * width + x);

                float sum_r = 0, sum_g = 0, sum_b = 0;
                float total_weight = flags->blurr_grid*flags->blurr_grid;
                int center = flags->blurr_grid / 2;

                // Create a dynamic weight matrix for the blur
                float weights[flags->blurr_grid][flags->blurr_grid];

                // Apply the kernel to the surrounding pixels
                for (int dy = -center; dy <= center; dy++) {
                    for (int dx = -center; dx <= center; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;

                        // Check bounds
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int neighbor_pixel = 3 * (ny * width + nx);
                            unsigned char red = image[neighbor_pixel];
                            unsigned char green = image[neighbor_pixel + 1];
                            unsigned char blue = image[neighbor_pixel + 2];

                            // Get the weight for the current neighbor
                            float weight = 1;

                            // Accumulate the weighted color values
                            sum_r += red * weight;
                            sum_g += green * weight;
                            sum_b += blue * weight;
                        }
                    }
                }

                // Normalize and store the resulting pixel color
                image_w_filter[pixel] = (unsigned char)(sum_r / total_weight);
                image_w_filter[pixel + 1] = (unsigned char)(sum_g / total_weight);
                image_w_filter[pixel + 2] = (unsigned char)(sum_b / total_weight);
            }
        }
        break;
    case 7: 
        for (int y = 0; y< height; y++) {
            for (int x = 0; x< width; x++) {
                int pixel = 3 * (y * width + x);
                float sum_r = 0, sum_g = 0, sum_b = 0;
                float total_weight = flags->blurr_grid*flags->blurr_grid;
                int center = flags->blurr_grid / 2;

                // Create a dynamic weight matrix for the blur
                float weights[flags->blurr_grid][flags->blurr_grid];
            if( y >= flags->y && y< flags->y + flags->zone && x >= flags->x && x<flags->x + flags->zone){
                // Apply the kernel to the surrounding pixels
                for (int dy = -center; dy <= center; dy++) {
                    for (int dx = -center; dx <= center; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;

                        // Check bounds
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int neighbor_pixel = 3 * (ny * width + nx);
                            unsigned char red = image[neighbor_pixel];
                            unsigned char green = image[neighbor_pixel + 1];
                            unsigned char blue = image[neighbor_pixel + 2];

                            // Get the weight for the current neighbor
                            float weight = 1;
                            //float weight = weights[dy + center][dx + center];

                            // Accumulate the weighted color values
                            sum_r += red * weight;
                            sum_g += green * weight;
                            sum_b += blue * weight;
                        }
                    }
                }
                                // Normalize and store the resulting pixel color
                image_w_filter[pixel] = (unsigned char)(sum_r / (total_weight +15));
                image_w_filter[pixel + 1] = (unsigned char)(sum_g / (total_weight +15));
                image_w_filter[pixel + 2] = (unsigned char)(sum_b / (total_weight +15));
            } else{
                unsigned char red = image[pixel];
         	    unsigned char green = image[pixel+1];
                unsigned char blue = image[pixel+2];
                image_w_filter[pixel] = red;
                image_w_filter[pixel + 1] = green;
                image_w_filter[pixel + 2] = blue; 
            }


            }
        }
        break;
        case 8:
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel = 3 * (y * width + x);

                // Accumulators for gradients
                float grad_x_r = 0, grad_x_g = 0, grad_x_b = 0;
                float grad_y_r = 0, grad_y_g = 0, grad_y_b = 0;

                // Define Sobel kernels
                int sobel_x[3][3] = {
                    {-1, 0, 1},
                    {-2, 0, 2},
                    {-1, 0, 1}
                };

                int sobel_y[3][3] = {
                    {-1, -2, -1},
                    { 0,  0,  0},
                    { 1,  2,  1}
                };

                // Apply Sobel kernel to surrounding pixels
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int nx = x + kx;
                        int ny = y + ky;

                        // Check bounds
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int neighbor_pixel = 3 * (ny * width + nx);
                            unsigned char red = image[neighbor_pixel];
                            unsigned char green = image[neighbor_pixel + 1];
                            unsigned char blue = image[neighbor_pixel + 2];

                            // Compute gradient contributions
                            grad_x_r += red * sobel_x[ky + 1][kx + 1];
                            grad_x_g += green * sobel_x[ky + 1][kx + 1];
                            grad_x_b += blue * sobel_x[ky + 1][kx + 1];

                            grad_y_r += red * sobel_y[ky + 1][kx + 1];
                            grad_y_g += green * sobel_y[ky + 1][kx + 1];
                            grad_y_b += blue * sobel_y[ky + 1][kx + 1];
                        }
                    }
                }

                // Combine gradients to calculate edge magnitude
                float edge_r = sqrt(grad_x_r * grad_x_r + grad_y_r * grad_y_r);
                float edge_g = sqrt(grad_x_g * grad_x_g + grad_y_g * grad_y_g);
                float edge_b = sqrt(grad_x_b * grad_x_b + grad_y_b * grad_y_b);

                // Clamp values to valid range (0-255)
                edge_r = fmin(fmax(edge_r, 0), 255);
                edge_g = fmin(fmax(edge_g, 0), 255);
                edge_b = fmin(fmax(edge_b, 0), 255);

                // Store the resulting pixel in the filtered image
                image_w_filter[pixel] = (unsigned char)edge_r;
                image_w_filter[pixel + 1] = (unsigned char)edge_g;
                image_w_filter[pixel + 2] = (unsigned char)edge_b;
            }
        }
        break;
        case 9: 
        float edge_factor = 1.5; // Factor de amplificación de bordes

        unsigned char* image_edge = (unsigned char*)malloc(width * height * 3);
        if (!image_edge) {
            fprintf(stderr, "Error al asignar memoria para la imagen de bordes.\n");
            break;
        }

        // Aplicar detección de bordes
        FilterOptions edge_flags = *flags;
        edge_flags.filter = 8; // Usar el filtro de detección de bordes
        process_image(&edge_flags, image_name, image, image_edge, width, height, channels);

        // Combinar la imagen original y la de bordes
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel = 3 * (y * width + x);

                // Valores originales
                unsigned char blurred_r = image[pixel];
                unsigned char blurred_g = image[pixel + 1];
                unsigned char blurred_b = image[pixel + 2];

                // Valores de bordes
                unsigned char edge_r = image_edge[pixel];
                unsigned char edge_g = image_edge[pixel + 1];
                unsigned char edge_b = image_edge[pixel + 2];

                // Aplicar la fórmula de desborronado
                int unblurred_r = (int)(blurred_r + edge_factor * edge_r);
                int unblurred_g = (int)(blurred_g + edge_factor * edge_g);
                int unblurred_b = (int)(blurred_b + edge_factor * edge_b);

                // Clampeo de valores
                image_w_filter[pixel] = (unsigned char)fmin(fmax(unblurred_r, 0), 255);
                image_w_filter[pixel + 1] = (unsigned char)fmin(fmax(unblurred_g, 0), 255);
                image_w_filter[pixel + 2] = (unsigned char)fmin(fmax(unblurred_b, 0), 255);
            }
        }

        // Liberar memoria temporal
        free(image_edge);
     break;

        
    }
        
    char image_w_filter_name[100];
    snprintf(image_w_filter_name, sizeof(image_w_filter_name), "%s_filter_%s", image_name, flags->filter_name);
    stbi_write_png(image_w_filter_name, width, height, channels, image_w_filter, width*channels);

}


int main (int argc, char** argv){

    FilterOptions *flags = malloc(sizeof(FilterOptions));
    if (arg_parser(argc, argv, flags)!=0){
    	return -1;
    }
    int width, height, channels;
    char * image_name = argv[1];
    unsigned char* image = stbi_load(image_name, &width, &height, &channels, 3);

    // Error cargando la imagen
        if (image == NULL || channels != 3) {
        fprintf(stderr,"Error al cargar la imagen\n");
        return 1;
    }

    unsigned char *image_w_filter = (unsigned char *)malloc(width * height * 3);
    process_image(flags, image_name, image, image_w_filter, width, height, channels);
    

    // Liberar memoria
    stbi_image_free(image);
    free(image_w_filter);
    
    return 0;
}


