#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#include "bitmap.h"

#define WIDTH 32
#define HEIGHT 32
#define NUM_COMPONENTS_BITS 64
#define IMAGE_BYTES 128
#define MAX_COMPONENT_PER_PCB 100


//functions prototypes
void template_mode(FILE *fp, int index_template);
void list_mode(FILE *fp, char* filename);
void connection_mode(FILE *fp, char* filename);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Invalid arguements!\n");
        exit(1);
    }

    char* mode = argv[1];
    char* templateFile = argv[2];
    char* outputImageFile = argv[3];

    if (strcmp(mode, "t") != 0 && strcmp(mode, "c") != 0 && strcmp(mode, "l") != 0) {
        fprintf(stderr, "Invalid mode selected!\n");
        exit(1);
    }

    FILE *fp = fopen(templateFile, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Can't load template file\n");
        exit(1);
    }
    if (strcmp(mode, "t") == 0) {
        int index_template = atoi(outputImageFile);
        template_mode(fp, index_template);
    }

    if (strcmp(mode, "l") == 0) {
        list_mode(fp, outputImageFile);
    }
    //pcbQualityControl(inputImageFile, templateFile, outputImageFile);

    if (strcmp(mode, "c") == 0) {
        connection_mode(fp, outputImageFile);
    }
    return 0;
}




//Functions
void template_mode(FILE *fp, int index_template) {
    int bytesRead;
    uint8_t byte;
    int num_components = 0;
    int max_num_components_bit = 0;
    int *num_components_arr = (int*)malloc(sizeof(int) * NUM_COMPONENTS_BITS);
    
    if ((bytesRead = fread(&byte, 1, 1, fp)) == 1) {
        for (int i = 7; i >= 0; i--) {
            num_components_arr[max_num_components_bit] = (byte >> i) & 1;
            max_num_components_bit++;
        }
    }
    int indices = 0;
    for (int i = max_num_components_bit - 1; i >= 0; i--) {
        num_components += num_components_arr[i] * pow(2, indices);
        indices++;
    }
    if (index_template >= num_components) {
        fprintf(stderr, "Template index out of range\n");
        exit(1);
    }
    free(num_components_arr);

    fseek(fp, ((IMAGE_BYTES * index_template) + 1), SEEK_SET);
    int bit[WIDTH * HEIGHT];
    int indexBit = 0;
    
    while ((bytesRead = fread(&byte, 1, 1, fp)) == 1 && indexBit < WIDTH * HEIGHT) {
        for (int i = 7; i >= 0; i--) {
            bit[indexBit] = (byte >> i) & 1;
            indexBit++;
        }
    }
    fclose(fp);
    printf("Template data:\n");
    int loop = 0;
    for (int i = indexBit - WIDTH; i >= -1; i++) {
        if (loop == WIDTH * HEIGHT) {
            printf("\n");
            break;
        }
        if (bit[i] == 0) {
            printf(" ");
        } else {
            printf("%d", bit[i]);
        }
        if ((i + 1) % WIDTH == 0 && loop != 0) {
            i -= (2 * WIDTH);
            printf("\n");
        }
        loop++;
    }
}


void list_mode(FILE *fp, char* filename) {
    //convert PCB image to binary data
    Bmp bmp = read_bmp(filename);
    int pcb_height = bmp.height;
    int pcb_width = bmp.width;
    int bmp_bin[pcb_height][pcb_width];
    for (int y = 0; y < pcb_height; y++) {
        for (int x = 0; x < pcb_width; x++) {
            double averageRGB = (bmp.pixels[y][x][RED] + bmp.pixels[y][x][BLUE] + bmp.pixels[y][x][GREEN]) / 3;
            if (averageRGB > 127) {
                bmp_bin[y][x] = 1;
            }
            else {
                bmp_bin[y][x] = 0;
            }
        }
    }

    //convert components image to binary data
    int bytesRead;
    uint8_t byte;
    int num_components = 0;
    int max_num_components_bit = 0;
    int *num_components_arr = (int*)malloc(sizeof(int) * NUM_COMPONENTS_BITS);
    fseek(fp, 0, SEEK_SET);
    if ((bytesRead = fread(&byte, 1, 1, fp)) == 1) {
        for (int i = 7; i >= 0; i--) {
            num_components_arr[max_num_components_bit] = (byte >> i) & 1;
            max_num_components_bit++;
        }
    }
    int indices = 0;
    for (int i = max_num_components_bit - 1; i >= 0; i--) {
        num_components += num_components_arr[i] * pow(2, indices);
        indices++;
    }
    free(num_components_arr);
    fseek(fp, 1, SEEK_SET);

    int bit_all_components[num_components][WIDTH * HEIGHT];

    for (int component_index = 0; component_index < num_components; component_index++) {
        fseek(fp, ((IMAGE_BYTES * component_index) + 1), SEEK_SET);
        int indexBit = 0;
        while ((bytesRead = fread(&byte, 1, 1, fp)) == 1 && indexBit < WIDTH * HEIGHT) {
            for (int i = 7; i >= 0; i--) {
                bit_all_components[component_index][indexBit] = (byte >> i) & 1;
                indexBit++;
            }
        }
    }
    int *found_compo_type = (int *)malloc(sizeof(int) * MAX_COMPONENT_PER_PCB);
    int *row_pos = (int *)malloc(sizeof(int) * MAX_COMPONENT_PER_PCB);
    int *col_pos = (int *)malloc(sizeof(int) * MAX_COMPONENT_PER_PCB);
    int num_found_components = 0;
    for (int row = 0; row <= pcb_height - HEIGHT; row++) {
        for (int col = 0; col <= pcb_width - WIDTH; col++) {
            for (int component_index = 0; component_index < num_components; component_index++) {
                bool isComponentPresent = true;

                for (int i = 0; i < HEIGHT; i++) {
                    for (int j = 0; j < WIDTH; j++) {
                        int pcbRow = row + i;
                        int pcbCol = col + j;
                        if (pcbRow < 0 || pcbRow >= pcb_height || pcbCol < 0 || pcbCol >= pcb_width) {
                            if (bit_all_components[component_index][i * WIDTH + j] == 1) {
                                isComponentPresent = false;
                            }
                        }
                        else if (bmp_bin[pcbRow][pcbCol] != bit_all_components[component_index][i * WIDTH + j]) {
                            isComponentPresent = false;
                        }
                    }
                }

                if (isComponentPresent) {
                    found_compo_type[num_found_components] = component_index;
                    row_pos[num_found_components] = row;
                    col_pos[num_found_components] = col;
                    num_found_components++;
                }
            }
        }
    }
    printf("Found %d components:\n", num_found_components);
    if (num_found_components != 0) {
        for (int i = 0; i < num_found_components; i++) {
            printf("type: %d, row: %d, column: %d\n", found_compo_type[i], row_pos[i], col_pos[i]);
        }
    }
    free(found_compo_type);
    free(row_pos);
    free(col_pos);
    free_bmp(bmp);
}


int **create_empty_grid(int pcb_height, int pcb_width) {
    int **grid = (int**)malloc(pcb_height*sizeof(int*));
    for (int i = 0; i < pcb_height; i++) {
        grid[i] = (int*)malloc(pcb_width*sizeof(int));
        for (int j = 0; j < pcb_width; j++) {
            grid[i][j] = 0;
        }
    }
    return grid;
}

void print_grid(int **grid, int pcb_height, int pcb_width) {
    for (int i = 0; i < pcb_height; i++) {
        for (int j = 0; j < pcb_width; j++) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
}

void free_grid(int **grid, int pcb_height, int pcb_width) {
    for (int i = 0; i < pcb_height; i++) {
        free(grid[i]);
    }
    free(grid);
}

int is_valid_cell(int row, int col, int pcb_height, int pcb_width) {
    if (row < 0 || col < 0 || row >= pcb_height || col >= pcb_width) {
        return 0;
    }
    return 1;
}

bool dfs(int **data, int **visited, int row, int col, int stop_row, int stop_col, int pcb_height, int pcb_width) {
    if (is_valid_cell(row, col, pcb_height, pcb_width) == 0 || visited[row][col] == 1) {
        return false;
    }
    visited[row][col] = 1;
    if (row == stop_row && col == stop_col) {
        return true;
    }
    if (is_valid_cell(row+1, col, pcb_height, pcb_width) == 1 && data[row+1][col] == data[row][col] && dfs(data, visited, row+1, col, stop_row, stop_col, pcb_height, pcb_width)) {
        return true;
    }
    else if (is_valid_cell(row-1, col, pcb_height, pcb_width) == 1 && data[row-1][col] == data[row][col] && dfs(data, visited, row-1, col, stop_row, stop_col, pcb_height, pcb_width)) {
        return true;
    }
    else if (is_valid_cell(row, col-1, pcb_height, pcb_width) == 1 && data[row][col-1] == data[row][col] && dfs(data, visited, row, col-1, stop_row, stop_col, pcb_height, pcb_width)) {
        return true;
    }
    else if (is_valid_cell(row, col+1, pcb_height, pcb_width) == 1 && data[row][col+1] == data[row][col] && dfs(data, visited, row, col+1, stop_row, stop_col, pcb_height, pcb_width)) {
        return true;
    }
    return false;
}

void boder_component(int** grid, int row, int col, int numRows, int numCols, int width, int height, int value) {
    for (int i = row; i < row + height && i < numRows; i++) {
        for (int j = col; j < col + width && j <= numCols; j++) {
            if (is_valid_cell(i, j, numRows, numCols) == 1) {
                grid[i][j] = value;
            }
        }
    }
}

void connection_mode(FILE *fp, char* filename) {
    //convert PCB image to binary data
    Bmp bmp = read_bmp(filename);
    int pcb_height = bmp.height;
    int pcb_width = bmp.width;
    int **bmp_bin = create_empty_grid(pcb_height, pcb_width);
    for (int y = 0; y < pcb_height; y++) {
        for (int x = 0; x < pcb_width; x++) {
            double averageRGB = (bmp.pixels[y][x][RED] + bmp.pixels[y][x][BLUE] + bmp.pixels[y][x][GREEN]) / 3;
            if (averageRGB > 127) {
                bmp_bin[y][x] = 1;
            }
            else {
                bmp_bin[y][x] = 0;
            }
        }
    }

    //convert components image to binary data
    int bytesRead;
    uint8_t byte;
    int num_components = 0;
    int max_num_components_bit = 0;
    int *num_components_arr = (int*)malloc(sizeof(int) * NUM_COMPONENTS_BITS);
    fseek(fp, 0, SEEK_SET);
    if ((bytesRead = fread(&byte, 1, 1, fp)) == 1) {
        for (int i = 7; i >= 0; i--) {
            num_components_arr[max_num_components_bit] = (byte >> i) & 1;
            max_num_components_bit++;
        }
    }
    int indices = 0;
    for (int i = max_num_components_bit - 1; i >= 0; i--) {
        num_components += num_components_arr[i] * pow(2, indices);
        indices++;
    }
    free(num_components_arr);
    fseek(fp, 1, SEEK_SET);

    int bit_all_components[num_components][WIDTH * HEIGHT];

    for (int component_index = 0; component_index < num_components; component_index++) {
        fseek(fp, ((IMAGE_BYTES * component_index) + 1), SEEK_SET);
        int indexBit = 0;
        while ((bytesRead = fread(&byte, 1, 1, fp)) == 1 && indexBit < WIDTH * HEIGHT) {
            for (int i = 7; i >= 0; i--) {
                bit_all_components[component_index][indexBit] = (byte >> i) & 1;
                indexBit++;
            }
        }
    }
    int *found_compo_type = (int *)malloc(sizeof(int) * 1);
    int *row_pos = (int *)malloc(sizeof(int) * 1);
    int *col_pos = (int *)malloc(sizeof(int) * 1);
    int num_found_components = 0;
    for (int row = 0; row <= pcb_height - HEIGHT; row++) {
        for (int col = 0; col <= pcb_width - WIDTH; col++) {
            for (int component_index = 0; component_index < num_components; component_index++) {
                bool isComponentPresent = true;

                for (int i = 0; i < HEIGHT; i++) {
                    for (int j = 0; j < WIDTH; j++) {
                        int pcbRow = row + i;
                        int pcbCol = col + j;
                        if (pcbRow < 0 || pcbRow >= pcb_height || pcbCol < 0 || pcbCol >= pcb_width) {
                            if (bit_all_components[component_index][i * WIDTH + j] == 1) {
                                isComponentPresent = false;
                            }
                        }
                        else if (bmp_bin[pcbRow][pcbCol] != bit_all_components[component_index][i * WIDTH + j]) {
                            isComponentPresent = false;
                        }
                    }
                }

                if (isComponentPresent) {
                    num_found_components++;
                    found_compo_type = (int *)realloc(found_compo_type, num_found_components * sizeof(int));
                    row_pos = (int *)realloc(row_pos, num_found_components * sizeof(int));
                    col_pos = (int *)realloc(col_pos, num_found_components * sizeof(int));
                    found_compo_type[num_found_components - 1] = component_index;
                    row_pos[num_found_components - 1] = row;
                    col_pos[num_found_components - 1] = col;
                }
            }
        }
    }
    //print_grid(bmp_bin, pcb_height, pcb_width);
    int **visited = create_empty_grid(pcb_height, pcb_width);
    printf("Found %d components:\n", num_found_components);
    if (num_found_components != 0) {
        for (int i = 0; i < num_found_components; i++) {
            printf("type: %d, row: %d, column: %d\n", found_compo_type[i], row_pos[i], col_pos[i]);
        }
        for (int component_check_for = 0; component_check_for < num_found_components; component_check_for++) {
            int start_row = row_pos[component_check_for];
            int start_col = col_pos[component_check_for];
            int foundConnect = 0;
            int *connection = (int *)malloc(num_found_components * sizeof(int));
            for (int component_connect = 0; component_connect < num_found_components; component_connect++) {
                if (component_connect == component_check_for) {
                    continue;
                }
                free_grid(visited, pcb_height, pcb_width);
                int **visited = create_empty_grid(pcb_height, pcb_width);

                int stop_row = row_pos[component_connect];
                int stop_col = col_pos[component_connect];
                // Set the entire border to 2
                for (int i = 0; i < num_found_components; i++) {
                    if (i != component_check_for && i != component_connect) {
                        boder_component(bmp_bin, row_pos[i], col_pos[i], pcb_height, pcb_width, WIDTH, HEIGHT, 2);
                    }
                }

                bool hasConnection = dfs(bmp_bin, visited, start_row, start_col, stop_row, stop_col, pcb_height, pcb_width);

                // Revert the changes by setting the border back to 1
                for (int i = 0; i < num_found_components; i++) {
                    if (i != component_check_for && i != component_connect) {
                        boder_component(bmp_bin, row_pos[i], col_pos[i], pcb_height, pcb_width, WIDTH, HEIGHT, 1);
                    }
                }

                if (hasConnection) {
                    connection[foundConnect] = component_connect;
                    foundConnect++;
                }
            }
            if (foundConnect > 0) {

                
                printf("Component %d connected to ", component_check_for);
                for (int i = 0; i < foundConnect; i++) {
                    printf("%d ", connection[i]);
                }
                printf("\n");
            }
            else {
                printf("Component %d connected to nothing\n", component_check_for);
            }
            free(connection);
        }
    }
    free_grid(bmp_bin, pcb_height, pcb_width);
    free(found_compo_type);
    free(row_pos);
    free(col_pos);
    free_bmp(bmp);
}
