#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hw.h"


int waitabit()
{
    for (int i = 0; i < 305; i++)
    {
      if (!hw_ping()) 
      {
	      printf("arduino not responding\n");
	      return 0;
      }
      sleep(1);
      printf(".");
      fflush(stdout);
    }
    return 1;
}

int main(int argc , char *argv[])
{
    while (1)
    {
      while (!init_hw(0)) { printf("."); fflush(stdout); }

      sleep(5);
      printf("red-warm...\n");
      set_color(1, 255, 255, 0, 0, 0);
      if (!waitabit()) continue; 

      printf("red...\n");
      set_color(1, 255, 0, 0, 0, 0);
      if (!waitabit()) continue; 

      printf("warm...\n");
      set_color(1, 0, 255, 0, 0, 0);
      if (!waitabit()) continue; 
    
      printf("off...\n");
      set_color(1, 0, 0, 0, 0, 0);
      sleep(1);
      close_hw(0);
      break;
    }

  return 0;
}

