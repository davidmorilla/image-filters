#include <stdlib.h>
#include <stdio.h>
#include <mpi.h> //cabecera mpi
#define STB_IMAGE_IMPLEMENTATION // Necesario para la implementaciÃ³n
#include "stb-master/stb_image.h"  
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb-master/stb_image_write.h"  
#include <string.h>
#include <stdint.h>


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
    float edge_factor;
    float resize_factor; // Scaling factor for resizing
} FilterOptions;

// Function to parse command-line arguments
int arg_parser(int argc, char **argv, FilterOptions *options) {

    if (strcmp(argv[1], "-help") == 0){
        printf("Usage: %s imagename [filter options]\n", argv[0]);
        printf("Filter options:\n");
        printf("  -help            : Show this help message.\n");
	printf("  -resize N        : Resize the image by a factor N (0 < N <= 5).\n");
        printf("  -bw              : Apply black-and-white filter.\n");
        printf("  -inverted        : Apply inverted color filter.\n");
        printf("  -r               : Apply red-channel filter.\n");
        printf("  -g               : Apply green-channel filter.\n");
        printf("  -b               : Apply blue-channel filter.\n");
        printf("  -rgb R G B       : Apply custom RGB filter (values 0-1 for each channel).\n");
        printf("  -blurr N         : Apply blur filter with grid size N (odd number).\n");
        printf("  -zoneblurr N X Y : Apply zone blur with grid size N (odd) at coordinates X, Y.\n");
        printf("  -edge            : Apply edge detection filter.\n");
        printf("  -unblurr         : Apply unblur filter.\n");
        exit(0);
    }

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
            if (argc - 1== ++i){
                printf("IF argc %i == %i + 1\n", argc, i);
                options->edge_factor = atof(argv[i]);
                
            } else{
                printf("ELSE argc %i == %i + 1\n", argc, i);
                options->edge_factor = 1;
            }
            options->filter = 9; 
        } else if (strcmp(argv[i], "-resize") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -resize flag requires a scaling factor (0 < N <= 5).\n");
                return -1;
            }
            options->resize_factor = atof(argv[++i]);
            if (options->resize_factor <= 0 || options->resize_factor > 5) {
                fprintf(stderr, "Error: Scaling factor must be in the range 0 < N <= 5.\n");
                return -1;
            }
            strcpy(options->filter_name, "resize");
            options->filter = 10; // Special value for resizing
        } else {
            fprintf(stderr, "Error: Unknown flag or argument %i: %s\n", i, argv[i]);
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
    float edge_factor;
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
                image_w_filter[pixel] = (unsigned char)(sum_r / (total_weight));
                image_w_filter[pixel + 1] = (unsigned char)(sum_g / (total_weight));
                image_w_filter[pixel + 2] = (unsigned char)(sum_b / (total_weight));
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
                                        /*
                    {0, 0, 0},
                    { 0,  0,  0},
                    { 0,  0,  0}   */            
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
        edge_factor = flags->edge_factor; // Factor de amplificaciÃ³n de bordes

        unsigned char* image_edge = (unsigned char*)malloc(width * height * 3);
        if (!image_edge) {
            fprintf(stderr, "Error al asignar memoria para la imagen de bordes.\n");
            break;
        }

        // Aplicar detecciÃ³n de bordes
        FilterOptions edge_flags = *flags;
        edge_flags.filter = 8; // Usar el filtro de detecciÃ³n de bordes
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

                // Aplicar la fÃ³rmula de desborronado
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
    case 10: // Resizing using bilinear interpolation
	    if (flags->resize_factor <= 0 || flags->resize_factor > 5) {
            fprintf(stderr, "Error: Resizing factor must be in the range (0 < n <= 5).\n");
            break;
	    }

	    // Calculate new dimensions
	    int new_width = (int)(width * flags->resize_factor);
	    int new_height = (int)(height * flags->resize_factor);
	    //image_w_filter= (unsigned char *)malloc(new_width * new_height * 3);
        

        //Get MPI rank and size
        int world_rank, world_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);

        if (world_rank == 0) {
            image_w_filter = (unsigned char*)malloc(new_width * new_height * 3);
        }

        // Calculate the number of rows each process will handle
        int rows_per_process = new_height / world_size;

        // Calculate the starting and ending row for each process
        int start_row = world_rank * rows_per_process;
        int end_row = (world_rank + 1) * rows_per_process;

        // Ensure the last process ends at the last row
        if (world_rank == world_size - 1) {
            end_row = new_height;
        }
        
        // Allocate local buffer for each process
        unsigned char* local_image;
        if (world_rank == world_size - 1) {
            local_image = (unsigned char*)malloc(new_width * (rows_per_process+ new_height%world_size) * 3);
        } else {
            local_image = (unsigned char*)malloc(new_width * rows_per_process * 3);
        }
        
	    // Perform bilinear interpolation
	    for (int y = start_row; y < end_row; y++) {
            for (int x = 0; x < new_width; x++) {
                // Map the pixel in the resized image to the original image
                float gx = x / flags->resize_factor;
                float gy = y / flags->resize_factor;

                // Get the integer and fractional parts
                int gxi = (int)gx;
                int gyi = (int)gy;
                float frac_x = gx - gxi;
                float frac_y = gy - gyi;

                // Ensure indices are within bounds
                int gxi1 = (gxi + 1 < width) ? gxi + 1 : gxi;
                int gyi1 = (gyi + 1 < height) ? gyi + 1 : gyi;

                for (int c = 0; c < channels; c++) {
                    // Fetch the four neighboring pixels
                    unsigned char top_left = image[(gyi * width + gxi) * channels + c];
                    unsigned char top_right = image[(gyi * width + gxi1) * channels + c];
                    unsigned char bottom_left = image[(gyi1 * width + gxi) * channels + c];
                    unsigned char bottom_right = image[(gyi1 * width + gxi1) * channels + c];

                    // Perform bilinear interpolation
                    float top = top_left + frac_x * (top_right - top_left);
                    float bottom = bottom_left + frac_x * (bottom_right - bottom_left);
                    float value = top + frac_y * (bottom - top);

                    // Assign the interpolated value to the new image
                    local_image[(y - start_row) * new_width * channels + x * channels + c] = (unsigned char)fminf(fmaxf(value, 0.0f), 255.0f);
                }
            }
	    }

         int *recvcounts = NULL;
    int *displs = NULL;

        if (world_rank == 0) {
            recvcounts = (int *)malloc(world_size * sizeof(int));
            displs = (int *)malloc(world_size * sizeof(int));

            int total_rows = 0;
            for (int i = 0; i < world_size; i++) {
                if (i == world_size - 1) {
                    recvcounts[i] = (rows_per_process + new_height % world_size) * new_width * 3;
                } else {
                    recvcounts[i] = rows_per_process * new_width * 3;
                }
                displs[i] = total_rows;
                total_rows += recvcounts[i];
            }
        }

        MPI_Gatherv(local_image, new_width * (end_row - start_row) * 3, MPI_UNSIGNED_CHAR, 
                image_w_filter, recvcounts, displs, MPI_UNSIGNED_CHAR, 
                0, MPI_COMM_WORLD);

        // Free local image buffer
        if (world_rank == 0) {
            free(recvcounts);
            free(displs);
        }

        free(local_image);
        width = new_width;
	    height = new_height;
        if (world_rank == 0) {
            char image_w_filter_name[100];
            snprintf(image_w_filter_name, sizeof(image_w_filter_name), "%s_filter_%s", image_name, flags->filter_name);
            save_as_bmp(image_w_filter_name, image_w_filter, width, height);
        } 
 
	    break; 
    }

    //stbi_write_jpg(image_w_filter_name, width, height, channels, image_w_filter, width*channels);
    

}






// BMP File Header (14 bytes)
// BMP File Header (14 bytes)
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;           // File type, should be 0x4D42 ('BM')
    uint32_t bfSize;           // File size in bytes
    uint16_t bfReserved1;      // Reserved
    uint16_t bfReserved2;      // Reserved
    uint32_t bfOffBits;        // Offset to pixel data
} BMPFileHeader;

// BMP DIB Header (40 bytes)
typedef struct {
    uint32_t biSize;           // Size of this header (40 bytes)
    int32_t biWidth;           // Image width in pixels
    int32_t biHeight;          // Image height in pixels
    uint16_t biPlanes;         // Number of color planes (should be 1)
    uint16_t biBitCount;       // Bits per pixel (should be 24)
    uint32_t biCompression;    // Compression type (0 for no compression)
    uint32_t biSizeImage;      // Size of image data
    int32_t biXPelsPerMeter;   // Horizontal resolution (pixels per meter)
    int32_t biYPelsPerMeter;   // Vertical resolution (pixels per meter)
    uint32_t biClrUsed;        // Number of colors used (0 for full color)
    uint32_t biClrImportant;   // Important colors (0 for all colors)
} BMPDIBHeader;
#pragma pack(pop)

// Function to save an image matrix as BMP
void save_as_bmp(const char *filename, uint8_t *image, int width, int height) {
    // Calculate padding (each row must be a multiple of 4 bytes)
    int row_padded = (width * 3 + 3) & (~3); // Row size rounded up to the nearest multiple of 4
    int padding = row_padded - width * 3;

    // File and DIB headers
    BMPFileHeader fileHeader;
    BMPDIBHeader dibHeader;

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPDIBHeader) + row_padded * height;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPDIBHeader);

    dibHeader.biSize = sizeof(BMPDIBHeader);
    dibHeader.biWidth = width;
    dibHeader.biHeight = -height; // Negative to store top-down (natural order)
    dibHeader.biPlanes = 1;
    dibHeader.biBitCount = 24;
    dibHeader.biCompression = 0; // BI_RGB
    dibHeader.biSizeImage = row_padded * height;
    dibHeader.biXPelsPerMeter = 2835; // 72 DPI
    dibHeader.biYPelsPerMeter = 2835; // 72 DPI
    dibHeader.biClrUsed = 0;
    dibHeader.biClrImportant = 0;

    // Open the file for writing
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Could not open file");
        return;
    }

    // Write headers
    fwrite(&fileHeader, sizeof(BMPFileHeader), 1, file);
    fwrite(&dibHeader, sizeof(BMPDIBHeader), 1, file);

    // Write pixel data with padding
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t red = image[(y * width + x) * 3 + 0];   // Red
            uint8_t green = image[(y * width + x) * 3 + 1]; // Green
            uint8_t blue = image[(y * width + x) * 3 + 2];  // Blue

            fwrite(&blue, 1, 1, file);  // BMP stores pixels in BGR format
            fwrite(&green, 1, 1, file);
            fwrite(&red, 1, 1, file);
        }
        // Write padding bytes
        uint8_t pad[3] = {0, 0, 0};
        fwrite(pad, 1, padding, file);
    }

    // Close the file
    fclose(file);
    printf("Image saved to %s\n", filename);
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


    // Inicializar MPI
	
    unsigned char *image_w_filter = (unsigned char *)malloc(width * height * 3);

    MPI_Init(NULL, NULL); 
    process_image(flags, image_name, image, image_w_filter, width, height, channels);
    MPI_Finalize(); //Finalizar mpi
    // Liberar memoria
    stbi_image_free(image);
    free(image_w_filter);
    
    return 0;
}
