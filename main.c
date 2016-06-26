#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "term.h"
#include "mbf.h"
#include "gif.h"

#define MIN(A, B)   ((A) < (B) ? (A) : (B))
#define MAX(A, B)   ((A) > (B) ? (A) : (B))


static struct Options {
    char *timings, *dialogue;
    char *output;
    float maxdelay, divisor;
    int loop;
    char *font;
    int height, width;
    int cursor;
    int quiet;
    int barsize;
    //----------
    uint32_t fps_sampling; //sampling framerate
    uint32_t num_frame; //frame counter
    uint8_t info; //flag wich rappresents if activate or not the info box
    uint32_t fps_replay; //reproducing framerate
    uint32_t x; //position of info box into screen
    uint32_t y; 
    uint8_t info_mode; //indicate how many info visualize
} options;


static struct fps_replay_regulator{

	double nominal_t;
	uint32_t eff_t;
	
}fps_replay_regulator;


uint32_t
_new_delay(struct fps_replay_regulator * obj,double fps_delay,uint32_t new_dt)
{
	//N.B. fps_delay is equal to  100/fps_replay

	obj->nominal_t += fps_delay;	//at each time i update the nominal delay

	uint32_t dt = (uint32_t)(obj->nominal_t - (double)obj->eff_t + 0.5);	//dt is the error beteen the nominal time and effective time, if i add 0.5 round  

	obj->eff_t += dt; // eff_t follow the nominal_t

	return dt; 
}



uint8_t
get_pair(Term *term, int row, int col)
{
    Cell cell;
    uint8_t fore, back;
    int inverse;

    /* TODO: add support for A_INVISIBLE */
    inverse = term->mode & M_REVERSE;
    if (term->mode & M_CURSORVIS)
        inverse = term->row == row && term->col == col ? !inverse : inverse;
    cell = term->addr[row][col];
    inverse = cell.attr & A_INVERSE ? !inverse : inverse;
    if (inverse) {
        fore = cell.pair & 0xF;
        back = cell.pair >> 4;
    } else {
        fore = cell.pair >> 4;
        back = cell.pair & 0xF;
    }
    if (cell.attr & (A_DIM | A_UNDERLINE))
        fore = 0x6;
    else if (cell.attr & (A_ITALIC | A_CROSSED))
        fore = 0x2;
    if (cell.attr & A_BOLD)
        fore |= 0x8;
    if (cell.attr & A_BLINK)
        back |= 0x8;
    return (fore << 4) | (back & 0xF);
}

void
draw_char(Font *font, GIF *gif, uint16_t code, uint8_t pair, int row, int col)//render the single char
{
    int i, j;
    int x, y;
    int index;
    int pixel;
    uint8_t *strip;

    index = get_index(font, code);
    if (index == -1)
        return;
    strip = &font->data[font->stride * font->header.h * index];
    y = font->header.h * row;
    for (i = 0; i < font->header.h; i++) {
        x = font->header.w * col;
        for (j = 0; j < font->header.w; j++) {
            pixel = strip[j >> 3] & (1 << (7 - (j & 7)));
            gif->cur[y * gif->w + x] = pixel ? pair >> 4 : pair & 0xF;
            x++;
        }
        y++;
        strip += font->stride;
    }
}

void
render(Term *term, Font *font, GIF *gif, uint16_t delay) //renderizza the frame of terminal
{
    int i, j;
    uint16_t code;
    uint8_t pair;

    for (i = 0; i < term->rows; i++) {
        for (j = 0; j < term->cols; j++) {
            code = term->addr[i][j].code;
            pair = get_pair(term, i, j); //return the foreground and back ground of each single char in the cell
            draw_char(font, gif, code, pair, i, j);
        }
    }
    add_frame(gif, delay); // i add the frame to the gif file
}

int
convert_script(Term *term,struct fps_replay_regulator * fpsrr)
{
    FILE *ft;
    int fd;
    float t;
    int n;
    uint8_t ch;
    Font *font;
    int w, h;
    int i, c;
    float d;
    uint16_t rd;
    float lastdone, done;
    char pb[options.barsize+1];
    GIF *gif;

    //--------
    double count; //used to count the time in float precision
    char* res;  //used to convert count into string
    char* num_frame; //counter of_number of frame
    double nominal_delay;   //rappresents the delay wich i want for each frame (100/FPS)
    uint32_t new_dt;  //reppresent the delay for each frame of the gif

    nominal_delay=(double)((double)100/(double)(options.fps_replay));
    new_dt=(uint32_t) nominal_delay; //i initialize new_dt with the int part of nominal_delay


    res=malloc(sizeof(char)*5);    //res is used to store the information about timing to put on the info box
    num_frame=malloc(sizeof(char)*3);

    ft = fopen(options.timings, "r");
    if (!ft) {
        fprintf(stderr, "error: could not load timings: %s\n", options.timings);
        goto no_ft;
    }
    fd = open(options.dialogue, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "error: could not load dialogue: %s\n", options.dialogue);
        goto no_fd;
    }
    font = load_font(options.font);
    if (!font) {
        fprintf(stderr, "error: could not load font: %s\n", options.font);
        goto no_font;
    }
    w = term->cols * font->header.w;
    h = term->rows * font->header.h;
    gif = new_gif(options.output, w, h, term->plt, options.loop);//prepare the GIF Header; Logic Screen Descriptor; Global Descriptor Table;Application Extension
    if (!gif) {
        fprintf(stderr, "error: could not create GIF: %s\n", options.output);
        goto no_gif;
    }
    /* discard first line of dialogue */
    do read(fd, &ch, 1); while (ch != '\n');
    if (options.barsize) {
        pb[0] = '[';
        pb[options.barsize-1] = ']';
        pb[options.barsize] = '\0';
        for (i = 1; i < options.barsize-1; i++)
            pb[i] = '-';
        lastdone = 0;
        printf("%s\r[", pb);
        /* get number of chunks */
        for (c = 0; fscanf(ft, "%f %d\n", &t, &n) == 2; c++);
        rewind(ft);/*The  rewind()  function sets the file position indicator for the stream
                     pointed to by stream to the beginning of the file.   It  is  equivalent
                    to:

                      (void) fseek(stream, 0L, SEEK_SET)*/
    }
    i = 0;
    while (fscanf(ft, "%f %d\n", &t, &n) == 2) {
        count=count+t; // tot time
        if (options.barsize) {
            done = i * (options.barsize-1) / c;
            if (done > lastdone) {
                while (done > lastdone) {
                    putchar('#');//putchar(c) is equivalent to putc(c, stdout).
                    lastdone++;
                }
                fflush(stdout);
            }
        }
        

        if((int)(options.fps_sampling*t)>1)
            for(int i=0;i<=(int)(options.fps_sampling*t);i++){

                
                sprintf(res,"%.2f",count);

                sprintf(num_frame,"%u",options.num_frame);
                
                if(options.info){
                
                    save_info(term,options.x,options.y,options.info_mode);// i save the previous state in the rectangle
                
                    add_info(term,res,num_frame,options.x,options.y,options.info_mode);//i put the info
                }

                new_dt=_new_delay(fpsrr,nominal_delay,new_dt); //i calculate the new delay

                
                render(term, font, gif, new_dt);//i render this image every 100/fps cent of sec only with the info
                
                if(options.info)
                    restore_term(term,options.x,options.y,options.info_mode);//i restore the rectangle
                
                options.num_frame++;
                
                
        }

        if(options.info)
	       restore_term(term,options.x,options.y,options.info_mode);
       
        while (n--) {
            read(fd, &ch, 1); //read n char from file foo.d
            parse(term, ch); //update the virtual terminal
        }
        if (!options.cursor)
            term->mode &= ~M_CURSORVIS;
        i++;
    }
    if (options.barsize) {
        while (lastdone < options.barsize-2) {
            putchar('#');
            lastdone++;
        }
        putchar('\n');
    }

    if(options.info)
        add_info(term,res,num_frame,options.x,options.y,options.info_mode);

    render(term, font, gif, options.fps_sampling);

    close_gif(gif);
    return 0;
no_gif:
    free(font);
no_font:
    close(fd);
no_fd:
    fclose(ft);
no_ft:
    return 1;
}

void
help(char *name)
{
    fprintf(stderr,
        "Usage: %s [options] timings dialogue\n\n"
        "timings:       File generated by script(1)'s -t option\n"
        "dialogue:      File generated by script(1)'s regular output\n\n"
        "options:\n"
        "  -o output    File name of GIF output\n"
        "  -l count     GIF loop count (0 = infinite loop)\n"
        "  -f font      File name of MBF font to use\n"
        "  -h lines     Terminal height\n"
        "  -w columns   Terminal width\n"
        "  -c on|off    Show/hide cursor\n"
        "  -q           Quiet mode (don't show progress bar)\n"
        "  -v           Verbose mode (show parser logs)\n"
        "  -i y|n            Put info on the gif, about # of frame and timing\n"
        "  -s sampling       Fix the sampling of the gif in terms of fps\n"
        "  -r reproduction   Fix the speed of replay in terms of fps speed \n"
        "  -x x_coord        Position of info box in the x-axis\n"
        "  -y y_coord        Position of info box in the y-axis\n"
        "  -n number_mode    1: Show only the time counter\n"
                             "                    2: Show only the frame counter\n"
                             "                    3: Show the time counter and frame counter (by Default)\n"
    , name);
}

void
set_defaults(struct winsize *size)
{

    options.output = "con.gif";
    options.maxdelay = FLT_MAX;
    options.divisor = 1.0;
    options.loop = -1;
    options.font = "misc-fixed-6x10.mbf";
    options.height = size->ws_row;
    options.width = size->ws_col;
    options.cursor = 1;
    options.quiet = 0;
    options.info=0;
    options.fps_sampling=30;
    options.fps_replay=30;
    options.num_frame=0;
    options.x=options.width-16;//i put the info box on the rigth side at the bottom by default
    options.y=options.height-2;
    options.info_mode=3;
}

int
main(int argc, char *argv[])
{
    int opt;
    int ret;
    Term *term;
    struct winsize size;
    struct fps_replay_regulator fpsrr;
    

    ioctl(0, TIOCGWINSZ, &size);
    set_defaults(&size);
    while ((opt = getopt(argc, argv, "o:m:d:l:f:h:w:c:s:i:r:x:y:n:qv")) != -1) { //i select the options
        switch (opt) {
        case 'o':
            options.output = optarg;
            break;
        case 'm':
            options.maxdelay = atof(optarg);
            break;
        case 'd':
            options.divisor = atof(optarg);
            break;
        case 'l':
            options.loop = atoi(optarg);
            break;
        case 'f':
            options.font = optarg;
            break;
        case 'h':
            options.height = atoi(optarg);
            break;
        case 'w':
            options.width = atoi(optarg);
            break;
        case 'c':
            if (!strcmp(optarg, "on") || !strcmp(optarg, "1"))
                options.cursor = 1;
            else if (!strcmp(optarg, "off") || !strcmp(optarg, "0"))
                options.cursor = 0;
            break;
        case 'q':
            options.quiet = 1;
            break;
        case 'v':
            set_verbosity(1);
            break;
        case 's':
            options.fps_sampling= atoi(optarg);
            break;
        case 'i':
        	if(optarg[0]!='y' && optarg[0] != 'n'){
        		printf("%s\n"," Insert -i y|n");
        		exit(0);
        	}
        	options.info=(optarg[0]=='y')?1:0;
        	break;
        case 'r':
        	options.fps_replay=atoi(optarg);
        	break;
        case 'x':
            options.x=options.x>=atoi(optarg)?atoi(optarg):options.x;
            break;
        case 'y':
            options.y=options.y>=atoi(optarg)?atoi(optarg):options.y;
            break;
        case 'n':
            options.info_mode=atoi(optarg);
            break;
        default:
            help(argv[0]);
            return 1;
        }
    }
    if (optind >= argc - 1) {
        fprintf(stderr, "%s: no input given\n", argv[0]);
        help(argv[0]);
        return 1;
    }
    options.timings = argv[optind++];
    options.dialogue = argv[optind++];
    options.barsize = options.quiet ? 0 : size.ws_col-1;
    term = new_term(options.height, options.width); //create a new terminal empty
    ret = convert_script(term,&fpsrr);
    free(term);
    return ret;
}
