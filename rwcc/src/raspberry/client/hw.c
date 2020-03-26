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

#include "hw.h"

#define SETCOLOR 'C'
#define SETMAXTIME 'M'
#define BUTTON_ON_REQUEST 'B'
#define BUTTON_OFF_REQUEST 'R'

// to make /dev/lamp available, put the following into a new file /etc/udev/rules.d/12-lamp.rules:
//
// # recognize arduino lamp controller and put it at /dev/lamp
// ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK="lamp"

static int serial[2] = { 0, 0 };
static char *port[2] = { "/dev/lamp", "/dev/rfcomm0" };
static int button_on = 0;
static int button_request = 0;

long msec()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return 1000L * tv.tv_sec + tv.tv_usec / 1000L;
}

int readchar(int handle, int timeout)
{
    int nread;
    char c;
    int tm = 0;

    long time1 = msec();

    do {
      if ((nread = read(serial[handle], &c, 1)) < 0)
      {
	    if (errno != EAGAIN)
            {
	      perror("could not read shake from arduino\n");
	      fflush(stderr);
	      return -2;
	    }
      }
      if (nread == 1) return c;
    } while (msec() - time1 < timeout);
    return -1;
}

int is_reset_signal()
{
  char c = readchar(0, 1);
  if (c == '*')
  {
	  c = readchar(0, 100);
	  if (c == '*') 
	  {
		  send_beep(2);
		  return 1;
	  }
  }
  return 0;
}

void send_beep(uint8_t n)
{
  uint8_t buf[2];
  buf[0] = 'T';
  buf[1] = n;

  if (serial[0] > 0) {
    if (write(serial[0], buf, 2) < 2) 
    {
       perror("could not write to arduino\n");
       fflush(stderr);
       close(serial[0]);
       serial[0] = 0;
       return;
    }
  }
}

int init_hw(int handle)
{
  char *shakebuf = "SHAKE?";

  serial[handle] = open(port[handle], O_RDWR | O_DSYNC | O_NONBLOCK);
  sleep(5);
  if (serial[handle] < 0) 
  {
	  printf("Not connected to arduino %d\n", handle);
	  fflush(stdout);
	  serial[handle] = 0;
	  return 0;
  }

  int written;
  for (int part = 0; part < 3; part++)
  {
    if ((written = write(serial[handle], shakebuf + (part * 2), 2)) < 2) 
    {
	  perror("could not shake with arduino\n");
	  printf("handle=%d\n", handle);
	  fflush(stderr);
	  fflush(stdout);
	  close(serial[handle]);
	  serial[handle] = 0;
	  return 0;
    }
    usleep(50000);
  }

  char responsebuf[7];
  for (int i = 0; i < 6; i++)
  {
    int cread;
    cread = readchar(handle, 500);
    if (cread < 0) 
    {
      close(serial[handle]);
      serial[handle] = 0;
      return 0;
    }
    responsebuf[i] = cread;
    if (responsebuf[i] == 'S') responsebuf[i=0] = 'S';
  }

  responsebuf[6] = 0;
  if (strcmp(responsebuf, "SHAKE!") == 0)
  {
	  printf("arduino %d connected\n", handle);
	  fflush(stdout);
	  if (handle == 1) send_beep(1);
	  return 1;
  }

  printf("arduino %d does not shake\n", handle);
  fflush(stdout);
  close(serial[0]);
  serial[0] = 0;
  return 0;
}


void set_color(uint8_t day, uint8_t r, uint8_t w, uint8_t c1, uint8_t c2, uint8_t maxtime)
{
  int written;
  uint8_t buf[6];
  buf[0] = SETCOLOR;
  buf[1] = day;
  buf[2] = r;
  buf[3] = w;
  buf[4] = c1;
  buf[5] = c2;

  if (serial[0] > 0) {
    if ((written = write(serial[0], buf, 6)) < 6) 
    {
       perror("could not write to arduino\n");
       fflush(stderr);
       close(serial[0]);
       serial[0] = 0;
       return;
    }
  }

  if (day == 0)
  {
    buf[0] = SETMAXTIME;
    buf[1] = maxtime;
    if (serial[0] > 0) {
      if ((written = write(serial[0], buf, 2)) < 2) 
      {
         perror("could not write to arduino\n");
         fflush(stderr);
	 close(serial[0]);
	 serial[0] = 0;
      }
    }
  }
}

int is_button_request()
{
  if (button_request)
  {
    button_request = 0;
    return 1;
  }
  if (serial[1] > 0)
  {
    int c = readchar(1, 1);
    if (c < -1) 
    {
      printf("button connection lost\n");
      close(serial[1]);
      serial[1] = 0;
      return 0;
    }
    if (c == '1') return 1;
  }
  return 0;
}

void send_button_request()
{
  uint8_t buf[1];
  if (button_on) buf[0] = BUTTON_OFF_REQUEST;
  else buf[0] = BUTTON_ON_REQUEST;
  button_on ^= 1;

  if (serial[0] > 0) {
    if (write(serial[0], buf, 1) < 1) 
    {
       perror("could not write to arduino\n");
       fflush(stderr);
       serial[0] = 0;
       close(serial[0]);
       return;
    }
  }
}

int button_alive()
{
  uint8_t buf[1];
  buf[0] = '@';

  if (serial[1] > 0) {
    if (write(serial[1], buf, 1) <= 0) 
    {
       perror("could not write to buttoino\n");
       fflush(stderr);
       close(serial[1]);
       serial[1] = 0;
       return 0;
    }
    int alive = readchar(1, 500);
    while (alive == '1') 
    {
      button_request = 1;
      alive = readchar(1, 500);
    }
    if (alive < 0) 
    {
      printf("button connection lost\n");
      close(serial[1]);
      serial[1] = 0;
      return 0;
    }
    if (alive != '!') 
    {
      printf("unexpected response from button: %c\n", alive);
      return 0;
    }
    return 1;
  }
  return 0;
}

int hw_ping()
{
  uint8_t buf = '@';

  if (serial[0] > 0) {
    if (write(serial[0], &buf, 1) < 1) 
    {
       perror("could not write to arduino\n");
       fflush(stderr);
       close(serial[0]);
       serial[0] = 0;
       return 0;
    }
  }
  char c = readchar(0, 100);
  if (c == '#') return 1;
  else return 0;
}

void close_hw(int handle)
{
   close(serial[handle]);
}

