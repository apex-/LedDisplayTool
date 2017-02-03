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
#include <sys/mman.h>                                                                                         
#include <sys/ioctl.h>                                                                                        
#include <sys/types.h>                                                                                        
#include <sys/stat.h>    
#include <time.h>

#define TESTDATA(x) 5*x


typedef struct {
  uint32_t *prev;
  uint32_t data; 
  uint32_t *next;
} node_t;



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
typedef uint32_t rgb_t;
typedef uint32_t hsl_t;


/** 
  * Given the values for hue (color), saturation and value
  * this function converts from HSV color space to RGB.
  * @param h hue [0 - 360] 
  * @param s saturation [0 - 1]
  * @param v value [0 - 1]
  * @return the RGB value encoded in an unsigned 32-bit integer: 0x00rrggbb
  */
rgb_t hsv_to_rgb(float h, float s, float v) {
    
    rgb_t rgb;
    if (s == 0) { // special case: no saturation -> shade of gray
        int i = v * 255;
        rgb = (i << 16) | (i << 8) | i;
    }
    int hi = h / 60.0;
    float f = h / 60.0 - hi;
    int pi = v * (1.0 - s) * 255;
    int qi = v * (1.0 - s * f) * 255;
    int ti = v * (1.0 - s * (1.0 - f)) * 255;
    int vi = v * 255;

    switch(hi) {
        case 0: 
        case 6: return (vi << 16 | ti << 8 | pi);
        case 1: return (qi << 16 | vi << 8 | pi);
        case 2: return (pi << 16 | vi << 8 | ti);
        case 3: return (pi << 16 | qi << 8 | vi);
        case 4: return (ti << 16 | pi << 8 | vi);
        case 5: return (vi << 16 | pi << 8 | qi);
        default:
            error_at_line(-1, 0, __FILE__, __LINE__, "Unexpected error (hue value out of bounds?)"); 
    }
    return -1;
}


int main(int argc, char ** argv) {

  int i= 0;
  uint64_t b = 5;
  uint32_t x = (uint32_t) ~0UL ^ 0xff00ff00;
 
  clockid_t clk_id;
  struct timespec t;

  char cmd[256];

  //for (i=0; i<10; i++) {
  //  printf("Time-step: %lu\n", get_time_step());
  //  usleep(100000L);
  //}

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

  rgb_t rgbvalue = hsv_to_rgb(h, s, v);
  printf("H:%f S:%f V:%f           RGB value: %x\n", h, s, v, rgbvalue);

}

