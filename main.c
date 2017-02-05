#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <math.h>
#include <fcntl.h>
#include <unistd.h> 
#include <assert.h> 
#include <stdint.h> 
#include <errno.h> 
#include <unistd.h> 
#include <signal.h>
#include <sys/mman.h> 
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <time.h>

#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"

#include "ws2811.h"

#define TARGET_FREQ                              WS2811_TARGET_FREQ
#define GPIO_PIN                                 18
#define DMA                                      5
#define STRIP_TYPE                               WS2811_STRIP_RGB  

#define WIDTH                                    8
#define HEIGHT                                   8
#define LED_COUNT                                (WIDTH * HEIGHT)

static uint8_t running = 1;
int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};


static void ctrl_c_handler(int signum)
{
    running = 0;
}


static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


/**
  * Returns the time difference between two consequtive
  * calls of this function in nanoseconds
  * 
  * @return time difference in nanoseconds
  */
long get_time_step() {

  static struct timespec t1;
  static struct timespec t2;

  // first call to function: initialise and return zero 
  if (t1.tv_sec == 0 && t1.tv_nsec == 0) {
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &t1)) {                                                                 
      perror("clock_gettime() failed."); 
    }
    return 0;
  }

  if (clock_gettime(CLOCK_MONOTONIC_RAW, &t2)) { 
    perror("clock_gettime() failed."); 
  } 
  
  long tdiffsec = t2.tv_sec - t1.tv_sec;
  long tdiffnsec = t2.tv_nsec - t1.tv_nsec;
  t1 = t2; 

  return tdiffsec * 1000000000L + tdiffnsec;
}

/** 
  * Given the values for hue (color), saturation and value
  * this function converts from HSV color space to RGB.
  * @param h hue [0 - 360] 
  * @param s saturation [0 - 1]
  * @param v value [0 - 1]
  * @return the RGB value encoded in an unsigned 32-bit integer: 0x00RRGGBB
  *         or -1 in case the supplied values were invalid.
  */
uint32_t hsv_to_rgb(float h, float s, float v) {
    
    if (h < 0.0 || h > 360.0) {
        return -1;	
    }
    if (s < 0.0 || s > 1.0) {
        return -1;
    }
    if (v < 0.0 || v > 1.0) {
        return -1;
    }

    uint32_t rgb;
    if (s == 0) { // no saturation: Return shade of gray
        uint32_t i = v * 255;
        return (i << 16) | (i << 8) | i;
    }
    uint32_t hi = h / 60.0;
    float f = h / 60.0 - hi;
    uint32_t pi = v * (1.0 - s) * 255;
    uint32_t qi = v * (1.0 - s * f) * 255;
    uint32_t ti = v * (1.0 - s * (1.0 - f)) * 255;
    uint32_t vi = v * 255;

    switch(hi) {
        case 0: 
        case 6: return (vi << 16 | ti << 8 | pi);
        case 1: return (qi << 16 | vi << 8 | pi);
        case 2: return (pi << 16 | vi << 8 | ti);
        case 3: return (pi << 16 | qi << 8 | vi);
        case 4: return (ti << 16 | pi << 8 | vi);
        case 5: return (vi << 16 | pi << 8 | qi);
        default:
            error_at_line(-1, 0, __FILE__, __LINE__, 
                "Unexpected error: Please report this to the developers."); 
	    return -1;
	}
}

static float globhue = 0.0;

void matrix_render(long tstep)
{
    int x, y;
    float tstepsec = tstep*1E-9;
    globhue += 10.0*tstepsec; 
    if (globhue > 360.0) {
        globhue -= 360.0;
    }

    for (x = 0; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            uint32_t rgb = hsv_to_rgb(globhue, 1.0, 0.1);
            ledstring.channel[0].leds[(y * width) + x] = rgb;
        }
    }
}


void matrix_clear(void)
{
    int x, y;

    for (y = 0; y < (height ); y++)
    {
        for (x = 0; x < width; x++)
        {
            ledstring.channel[0].leds[(y * width) + x] = 0x00000000;
        }
    }
}


int main(int argc, char ** argv) {

    char cmd[256];
    int ret;

    setup_handlers();

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    while(running) {

        long tstep = get_time_step();        

        matrix_render(tstep);        

        if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
        {
            fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
            break;
        }

        /*
        if (fgets(cmd, sizeof(cmd),  stdin) != NULL) {
            //printf("Command: %s", cmd);
        }   

        char *pch;
        int ti = 0;
        char *arg[5];
        float h, s, v;
        char *errCheck;

        pch = strtok(cmd," \n");
    
        while (pch != NULL) {
        arg[ti] = pch;
    
        if(ti == 1) {
            h = strtof(arg[1], &errCheck);
            if (errCheck == arg[1]) {
                printf("Conversion error 1 %s\n", arg[1]);
            }
        }
        if(ti == 2) {
            s = strtof(arg[2], &errCheck);
            if (errCheck == arg[2]) {
                printf("Conversion error 2 %s\n", arg[2]);
            }
        }
        if(ti == 3) {
            v = strtof(arg[3], &errCheck);
            if (errCheck == arg[3]) {
                printf("Conversion error 3 %s\n", arg[3]);
            }
        }
        pch = strtok(NULL," \n");
        ti++;
        } 

        uint32_t rgb = hsv_to_rgb(h, s, v);
        printf("H:%f S:%f V:%f           RGB value: %x\n", h, s, v, rgb);
   
        */
        long tsleep = (1000000000L-tstep) / 30; 
        
        usleep(tsleep/1000);
    }

    matrix_clear();
    //matrix_render();
    ws2811_render(&ledstring);

    ws2811_fini(&ledstring);

}

