#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define SETCOLOR 'C'

static int serial = 0;

long msec()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return 1000L * tv.tv_sec + tv.tv_usec / 1000L;
}

int readchar(int timeout)
{
    int nread;
    char c;
    int tm = 0;

    long time1 = msec();

    do {
      if ((nread = read(serial, &c, 1)) < 0)
      {
	    if (errno != EAGAIN)
            {
	      perror("could not read shake from arduino\n");
	      fflush(stderr);
	      return -1;
	    }
      }
      if (nread == 1) return c;
    } while (msec() - time1 < timeout);
    return -1;
}

int init_hw()
{
  char *shakebuf = "SHAKE?";

  serial = open("/dev/ttyUSB0", O_RDWR | O_DSYNC | O_NONBLOCK);
  sleep(5);
  if (serial < 0) 
  {
	  printf("Not connected to arduino\n");
	  fflush(stdout);
	  return 0;
  }

  int written;
  if ((written = write(serial, shakebuf, 6)) < 0) 
  {
	  perror("could not shake with arduino\n");
	  fflush(stderr);
  }

  char responsebuf[7];
  for (int i = 0; i < 6; i++)
  {
    int cread;
    cread = readchar(500);
    if (cread < 0) return 0;
    responsebuf[i] = cread;
    if (responsebuf[i] == 'S') responsebuf[i=0] = 'S';
  }

  responsebuf[6] = 0;
  if (strcmp(responsebuf, "SHAKE!") == 0)
  {
	  printf("arduino connected\n");
	  fflush(stdout);
	  return 1;
  }

  printf("arduino does not shake\n");
  fflush(stdout);
  return 0;
}


void set_color(uint8_t day, uint8_t r, uint8_t w, uint8_t c1, uint8_t c2)
{
  int written;
  uint8_t buf[6];
  buf[0] = SETCOLOR;
  buf[1] = day;
  buf[2] = r;
  buf[3] = w;
  buf[4] = c1;
  buf[5] = c2;

  if (serial > 0) {
    if ((written = write(serial, buf, 6)) < 0) 
    {
       perror("could not write to arduino\n");
       fflush(stderr);
    }
  }
}

void close_hw()
{
   close(serial);
}
