#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gif.h"

/* helper to write a little-endian 16-bit number portably */
#define write_num(fd, n) write((fd), (uint8_t []) {(n) & 0xFF, (n) >> 8}, 2)

struct Node {
    uint16_t key;
    struct Node *children[0x10]; //16 children
};
typedef struct Node Node;

static Node *
new_node(uint16_t key)
{
    Node *node = calloc(1, sizeof(*node));
    if (node)
        node->key = key;
    return node;
}

static void
del_trie(Node *root)
{
    int i;

    if (!root)
        return;
    for (i = 0; i < 0x10; i++)
        del_trie(root->children[i]);
    free(root);
}

static void put_loop(GIF *gif, uint16_t loop);

GIF *
new_gif(const char *fname, uint16_t w, uint16_t h, uint8_t *gct, int loop) //prepare the GIF Header; Logic Screen Descriptor; Global Descriptor Table;Application Extension
{
    GIF *gif = calloc(1, sizeof(*gif) + 2*w*h);
    if (!gif)
        goto no_gif; //jump to no_gif
    gif->w = w; gif->h = h;
    gif->cur = (uint8_t *) &gif[1];
    gif->old = &gif->cur[w*h];
    /* fill back-buffer with invalid pixels to force overwrite */
    memset(gif->old, 0x10, w*h);
    gif->fd = creat(fname, 0666);
    if (gif->fd == -1)
        goto no_fd; //jump to no_fd
    write(gif->fd, "GIF89a", 6); //GIF header
    /*Logic Screen Descriptor*/
    write_num(gif->fd, w);//16bit for Width
    write_num(gif->fd, h);//16bit for heigth
    write(gif->fd, (uint8_t []) {0xF3, 0x00, 0x00}, 3);//24bit for: Packet field; Background Color index;Pixel aspect Ratio 
    write(gif->fd, gct, 0x30);//Global Descriptor Table of 48 bit
    if (loop >= 0 && loop <= 0xFFFF)
        put_loop(gif, (uint16_t) loop); // put the application extension for looping the gif
    return gif;
no_fd:
    free(gif);
no_gif:
    return NULL;
}

static void
put_loop(GIF *gif, uint16_t loop) //is used to specify the loop counter in riproduction of gif 
{   
    /*Application Extension*/
    write(gif->fd, (uint8_t []) {'!', 0xFF, 0x0B}, 3); // '!' == 0x21
    write(gif->fd, "NETSCAPE2.0", 11);
    write(gif->fd, (uint8_t []) {0x03, 0x01}, 2);
    write_num(gif->fd, loop);
    write(gif->fd, "\0", 1); //End of Application extension part
}

/* Add packed key to buffer, updating offset and partial.
 *   gif->offset holds position to put next *bit*
 *   gif->partial holds bits to include in next byte */
static void
put_key(GIF *gif, uint16_t key, int key_size)
{
    int byte_offset, bit_offset, bits_to_write;
    byte_offset = gif->offset / 8;
    bit_offset = gif->offset % 8;
    gif->partial |= ((uint32_t) key) << bit_offset;
    bits_to_write = bit_offset + key_size;
    while (bits_to_write >= 8) {
        gif->buffer[byte_offset++] = gif->partial & 0xFF;
        if (byte_offset == 0xFF) { //when i reach the maximum size 0xFF = 255 byte i flush the gif->buffer to the gif file 
            write(gif->fd, "\xFF", 1);
            write(gif->fd, gif->buffer, 0xFF); // write the buffer to the file
            byte_offset = 0;
        }
        gif->partial >>= 8;
        bits_to_write -= 8;
    }
    gif->offset = (gif->offset + key_size) % (0xFF * 8);
}

static void
end_key(GIF *gif)
{
    int byte_offset;
    byte_offset = gif->offset / 8;
    gif->buffer[byte_offset++] = gif->partial & 0xFF;
    write(gif->fd, (uint8_t []) {byte_offset}, 1);
    write(gif->fd, gif->buffer, byte_offset);
    write(gif->fd, "\0", 1);
    gif->offset = gif->partial = 0;
}

static void
put_image(GIF *gif, uint16_t w, uint16_t h, uint16_t x, uint16_t y) //put a frame into a gif file
{
    int nkeys, key_size, i, j;
    Node *node, *child, *root;

    root = malloc(sizeof(*root));
    write(gif->fd, ",", 1); // ,==2C => Start of Image descriptor
    write_num(gif->fd, x); //Image Left
    write_num(gif->fd, y); //Image Top
    write_num(gif->fd, w); //Image width
    write_num(gif->fd, h); //Image weigth
    write(gif->fd, (uint8_t []) {0x00, 0x04}, 2); //Packet Field
    /* Create nodes for single pixels. */
    for (nkeys = 0; nkeys < 0x10; nkeys++)//up to 16 nkeys
        root->children[nkeys] = new_node(nkeys); //allocate 16 new children
    node = root;
    key_size = 5;
    nkeys += 2; /* skip clear code and stop code */
    put_key(gif, 0x10, key_size); /* clear code */ //The clear code specify to decompressor the start of compress data with lzw and specify also that we need to reinitialize the code table
    for (i = y; i < y+h; i++) {
        for (j = x; j < x+w; j++) {
#ifdef CROP
            uint8_t pixel = 2;
#else
            uint8_t pixel = gif->cur[i*gif->w+j];
#endif
            child = node->children[pixel];
            if (child) {
                node = child;
            } else {
                put_key(gif, node->key, key_size);
                if (nkeys < 0x1000) {
                    if (nkeys == (1 << key_size))
                        key_size++;
                    node->children[pixel] = new_node(nkeys++);
                }
                node = root->children[pixel];
            }
        }
    }
    put_key(gif, node->key, key_size);
    put_key(gif, 0x11, key_size); /* stop code */ // specify the end of information code (EOI) => end of compressed data 
    end_key(gif);
    del_trie(root);
}

static int
get_bbox(GIF *gif, uint16_t *w, uint16_t *h, uint16_t *x, uint16_t *y)//i scan the image pixel per pixel in order to find if the image change
{
    int i, j, k;
    int left, right, top, bottom;
    left = gif->w; right = 0;
    top = gif->h; bottom = 0;
    k = 0;
    for (i = 0; i < gif->h; i++) {
        for (j = 0; j < gif->w; j++, k++) {
            if (gif->cur[k] != gif->old[k]) {//if i found a change i set the local variable left, right, top, bottom
                if (j < left)   left    = j;
                if (j > right)  right   = j;
                if (i < top)    top     = i;
                if (i > bottom) bottom  = i;
            }
        }
    }
    if (left != gif->w && top != gif->h) {//equal to if i have a changed
        *x = left; *y = top;
        *w = right - left + 1;
        *h = bottom - top + 1;
        return 1;
    } else {
        return 0;// if the image don't change
    }
}

static void
set_delay(GIF *gif, uint16_t d) //set delay into graphics color extention at the beginning of each frame
{
#ifdef CROP
    write(gif->fd, (uint8_t []) {'!', 0xF9, 0x04, 0x08}, 4);
#else
    write(gif->fd, (uint8_t []) {'!', 0xF9, 0x04, 0x04}, 4);
#endif
    write_num(gif->fd, d); //i put the delay time relative to this frame into the graphics color extention table
    write(gif->fd, "\0\0", 2);//end of graphics color extention
}

void
add_frame(GIF *gif, uint16_t d)//put the frame into the gif box
{
    uint16_t w, h, x, y;
    uint8_t *tmp;

    if (d)
        set_delay(gif, d);
    if (!get_bbox(gif, &w, &h, &x, &y)) {
        /* image's not changed; save one pixel just to add delay */
        w = h = 1;
        x = y = 0;
    }
    put_image(gif, w, h, x, y);
    tmp = gif->old;
    gif->old = gif->cur;
    gif->cur = tmp;
}

void
close_gif(GIF* gif)
{
    write(gif->fd, ";", 1);
    close(gif->fd);
    free(gif);
}
