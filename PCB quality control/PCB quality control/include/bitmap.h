#ifndef _BITMAP_H
#define _BITMAP_H

// NOTE: you do not need to edit this file

#define RED 0
#define GREEN 1
#define BLUE 2

// Here we define our own type "Bmp"
// it is a struct containing all the data about an image
typedef struct {

    // The height of the image in pixels
    unsigned int height; 

    // The width of the image in pixels
    unsigned int width; 

    // 2D array of pointers to pixels
    // pixels are a 3 byte arrays of [RED, GREEN, BLUE]
    // each is a colour (from 0-255) is that component of colour in the pixel
    unsigned char ***pixels;

    // Don't worry about this, we just use it to store some extra information about the image
    void *header;
} Bmp;

// Open an image
Bmp read_bmp(char *filename); 

// Write an image to a file
void write_bmp(Bmp, char *filename);

// Copy an image
Bmp copy_bmp(Bmp bmp);

// Free an image
// Make sure this is called once for every Bmp you create
void free_bmp(Bmp);


#endif
