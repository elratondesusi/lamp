#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hw.h"

#define max_packet_size 200000

#define MAX_PLAN_SIZE 200
#define PLAN_PERSISTENT_FILE "/home/pi/plan.cfg"
#define PLANON_PERSISTENT_FILE "/home/pi/plan.onoff"

static char *login_request_packet = "lampa calling, shake hand?";
static char *login_response_packet = "welcome lampa, hand shake!";

static int socket_desc;
static volatile int connected = 0;

static volatile int plan_is_on = 1;

//format plan
//HH:MM RWCC_COLOR MODE
//mode: 0 - set color at this time
//      1 - interpolate to this color from previous time
//RWCC_COLOR: RR WW C1 C2  in hex
//lines starting with # and empty lines are comments
//example:
//00:10 FF 00 30 30 0
//  at 00:10 will set color to red=FF, warm=00, cold1=30, cold2=30

static volatile int plan_len;
static volatile int plan_hour[MAX_PLAN_SIZE];
static volatile int plan_minute[MAX_PLAN_SIZE];
static volatile unsigned int plan_red[MAX_PLAN_SIZE];
static volatile unsigned int plan_warm[MAX_PLAN_SIZE];
static volatile unsigned int plan_cold1[MAX_PLAN_SIZE];
static volatile unsigned int plan_cold2[MAX_PLAN_SIZE];
static volatile int plan_interpolate[MAX_PLAN_SIZE];
static volatile int adopt_new_plan;

static int cigs_red_1, cigs_warm_1, cigs_cold1_1, cigs_cold2_1, 
           cigs_red_2, cigs_warm_2, cigs_cold1_2, cigs_cold2_2;
static double cigs_start_time, cigs_total_time;

static char packet[max_packet_size];
static int len_pckt;

static int current_step;
static int current_interpolated_goal_step;
static uint8_t current_red, current_warm, current_cold1, current_cold2;

static char *setcolor_packet = "setcolor";
static char *planon_packet = "planon";
static char *planoff_packet = "planoff";
static char *plantest_packet = "plantest";
static char *newplan_packet = "newplan:";
static char *bim_packet = "bim";
static char *bam_packet = "bam";

void save_planon_status();
void save_new_plan_to_file();

char *current_time()
{
  static char curtime[100];
  time_t tim;
  time(&tim);
  ctime_r(&tim, curtime);
  curtime[strlen(curtime) - 1] = 0;
  return curtime;
}

void set_new_plan(char *newplan)
{
  plan_len = 0;
  while (*newplan)
  {
    while ((*newplan == '\n') || (*newplan == '\r')) newplan++;
    if (! *newplan) break;

    if (*newplan != '#') 
    { 
      int hour, minute;
      uint32_t color;
      sscanf(newplan, "%d:%d %x %x %x %x %d\n",
             &plan_hour[plan_len],
             &plan_minute[plan_len],
             &plan_red[plan_len],
             &plan_warm[plan_len],
             &plan_cold1[plan_len],
             &plan_cold2[plan_len],
             &plan_interpolate[plan_len]);
      plan_len++;
    }
    while ((*newplan) && (*(++newplan) != '\n'));
  } 
  printf("%s: loaded plan with %d steps\n", current_time(), plan_len);
  for (int i = 0; i < plan_len; i++)
    printf("%02d:%02d %02x %02x %02x %02x %d\n", plan_hour[i], plan_minute[i], plan_red[i], plan_warm[i], plan_cold1[i], plan_cold2[i], plan_interpolate[i]);
  printf("---");
  fflush(stdout);
  adopt_new_plan = 1;
}

int send_data(char *data, int len)
{
        int already_sent = 0;
        int sent;
        while (already_sent < len)
        {
          if ((sent = send(socket_desc, data + already_sent, len - already_sent, MSG_NOSIGNAL)) < 0)
          {
            perror("could not send to socket");
            return 1;
          }
          already_sent += sent;
        }
        return 0;
}

int recv_data(char *data, int len)
{
        int already_recv = 0;
        int recvd;
        while (already_recv < len)
        {
           if ((recvd = recv(socket_desc, data + already_recv, len - already_recv, 0)) < 0)
           {
              perror("could not recv from socket");
              return 1;
           }
           already_recv += recvd;
        }
        return 0;
}

int send_packet(char *packet, int packet_len)
{
        char len[4];
        len[0] = packet_len & 255;
        len[1] = (packet_len >> 8) & 255;
        len[2] = len[3] = 0;

        if (send_data(len, 4)) return 1;
        if (send_data(packet, packet_len)) return 1;

        return 0;
}

int recv_packet()
{
        unsigned char *pckt = (unsigned char *)packet;
        if (recv_data(packet, 4)) return 1;
        len_pckt = pckt[0] + (pckt[1] << 8);
        //printf("packet[%d]\n", len_pckt);
        if (len_pckt > max_packet_size - 1)
        {
          printf("packet size exceeded\n");
	  fflush(stdout);
          return 1;
        }
        if (recv_data(packet, len_pckt)) return 1;
        packet[len_pckt] = 0;
        return 0;
}

void process_packet()
{
  //printf("process packet %s\n", packet);
  if (strncmp(packet, setcolor_packet, strlen(setcolor_packet)) == 0)
  {
     int day, red, warm, cold1, cold2, maxtime;
     sscanf(packet + strlen(setcolor_packet) + 1, "%d,%d,%d,%d,%d,%d)", &day, &red, &warm, &cold1, &cold2, &maxtime);
     printf("%s: setcolor(%d,%d,%d,%d,%d,%d)\n", current_time(), day, red, warm, cold1, cold2, maxtime);
     fflush(stdout);
     set_color(day, red, warm, cold1, cold2, maxtime);
  }
  else if (strncmp(packet, planon_packet, strlen(planon_packet)) == 0)
  {
    printf("%s: planon\n", current_time());
    fflush(stdout);
    plan_is_on = 1;
    save_planon_status();
  }
  else if (strncmp(packet, planoff_packet, strlen(planoff_packet)) == 0)
  {
    printf("%s: planoff\n", current_time());
    fflush(stdout);
    plan_is_on = 0;
    save_planon_status();
  }
  else if (strncmp(packet, plantest_packet, strlen(plantest_packet)) == 0)
  {
    printf("%s: plantest\n", current_time());
    fflush(stdout);
  }
  else if (strncmp(packet, newplan_packet, strlen(newplan_packet)) == 0)
  {
    printf("%s: newplan\n", current_time());
    fflush(stdout);
    set_new_plan(packet + strlen(newplan_packet));
    save_new_plan_to_file();
  }
  else if (strncmp(packet, bim_packet, strlen(bim_packet)) == 0)
  {
    send_packet(bam_packet, strlen(bam_packet));
  }
  else { printf("%s: unknown packet %s\n", current_time(), packet); fflush(stdout); }
}

long time_difference_in_secs(int h1, int m1, int h2, int m2)
{
	long minute_diff = m2 - m1;
	if (minute_diff < 0) 
	{
		minute_diff += 60;
		h1++;
	}
	long hour_diff = h2 - h1;
	if (hour_diff < 0) hour_diff += 24;
	return (hour_diff * 60 + minute_diff) * 60;
}

void take_plan_step(int i, time_t t)
{
  current_step = i;
  if (plan_is_on)
    set_color(1, plan_red[i], plan_warm[i], plan_cold1[i], plan_cold2[i], 0);
  char step_description[100];
  sprintf(step_description, "status step %02d [%02x,%02x,%02x,%02x,%d] plan on: %d", i, plan_red[i], plan_warm[i], plan_cold1[i], plan_cold2[i], plan_interpolate[i], plan_is_on);
  send_packet(step_description, strlen(step_description));
  printf("%s: %s\n", current_time(), step_description);
  fflush(stdout);

  int next_i = (i + 1) % plan_len;

  if (plan_interpolate[next_i] == 0) 
    current_interpolated_goal_step = -1;
  else
  {
    current_interpolated_goal_step = next_i;
    cigs_red_1 = plan_red[i];
    cigs_warm_1 = plan_warm[i];
    cigs_cold1_1 = plan_cold1[i];
    cigs_cold2_1 = plan_cold2[i];
    cigs_red_2 = plan_red[next_i];
    cigs_warm_2 = plan_warm[next_i];
    cigs_cold1_2 = plan_cold1[next_i];
    cigs_cold2_2 = plan_cold2[next_i];
    cigs_start_time = t;
    cigs_total_time = time_difference_in_secs(plan_hour[i], plan_minute[i], plan_hour[next_i], plan_minute[next_i]);
  }
}

void *button_thread(void *args)
{
  while (1)
  {
    while (!init_hw(1)) sleep(5);
    int alive_check_counter = 0;
    while (1)
    {
      if (alive_check_counter++ > 100)
      {
        alive_check_counter = 0;
        if (!button_alive())
	{
          printf("%s: lost button\n", current_time());
	  close_hw(1);
	  break;
	}
      }

      if (is_button_request()) 
      {
	char statusmsg[40];
        send_button_request();
	printf("%s: button\n", current_time());
	sprintf(statusmsg, "status button pressed, plan on: %d", plan_is_on); 
        send_packet(statusmsg, strlen(statusmsg));
      }

      usleep(200000UL);
    }
  }
}

void *agenda_thread(void *args)
{
  adopt_new_plan = 0;
  while (connected)
  {
    if (is_reset_signal()) 
    {
	printf("%s: manual shutdown\n", current_time());
	fflush(stdout);
        send_packet("manual shutdown", 15);
	usleep(200000);

        close_hw(0);
        close(socket_desc);

        save_planon_status();
	usleep(400000);

	execlp("sudo", "sudo", "shutdown", "now", 0);
    }

    sleep(1);
    time_t t;
    time(&t);
    struct tm *now = localtime(&t);
    int hour = now->tm_hour;
    int minute = now->tm_min;
    int second = now->tm_sec;

    if (adopt_new_plan)
    {
	    long min_time_dist = 24 * 60 * 60;
	    int min_plan_i = 0;
	    for (int i = 0; i < plan_len; i++)
	    {
		  long time_dist = time_difference_in_secs(plan_hour[i], plan_minute[i], hour, minute);
		  if (time_dist < min_time_dist)
		  {
			min_time_dist = time_dist;
		        min_plan_i = i;
		  }	
	    }
	    take_plan_step(min_plan_i, t); 
	    adopt_new_plan = 0;
    }

    for (int i = 0; i < plan_len; i++)
    {
      if ((plan_hour[i] == hour) && (plan_minute[i] == minute) && (second < 10) && (current_step != i)) 
      {
        take_plan_step(i, t);

        break;
      }
    }

    if (current_interpolated_goal_step > 0)
    {
	    long cigs_seconds_from_start = t - cigs_start_time;
	    double cigs_advancement = cigs_seconds_from_start / (double)cigs_total_time;
	    double cigs_adv_inverse = 1 - cigs_advancement;

	    int red_now = (int)(0.5 + cigs_red_1 * cigs_adv_inverse + cigs_red_2 * cigs_advancement);
            int warm_now = (int)(0.5 + cigs_warm_1 * cigs_adv_inverse + cigs_warm_2 * cigs_advancement);
	    int cold1_now = (int)(0.5 + cigs_cold1_1 * cigs_adv_inverse + cigs_cold1_2 * cigs_advancement);
	    int cold2_now = (int)(0.5 + cigs_cold2_1 * cigs_adv_inverse + cigs_cold2_2 * cigs_advancement);
	    if (plan_is_on)
	      set_color(1, red_now, warm_now, cold1_now, cold2_now, 0);
            printf("%s: %02d [%02x,%02x,%02x,%02x]\n", current_time(), current_interpolated_goal_step, red_now, warm_now, cold1_now, cold2_now);
            fflush(stdout);
    }
  }
}

void start_agenda_thread()
{
  pthread_t t;
  if (pthread_create(&t, 0, agenda_thread, 0) != 0)
  {
    perror("could not start thread");
  }
  printf("agenda thread started\n");
  fflush(stdout);
}

void start_button_thread()
{
  pthread_t t;
  if (pthread_create(&t, 0, button_thread, 0) != 0)
  {
    perror("could not start button thread");
  }
  printf("button thread started\n");
  fflush(stdout);
}

void save_planon_status()
{
  FILE *f = fopen(PLANON_PERSISTENT_FILE, "w+");
  if (f == 0)
  {
    perror("could not open plan status file for writing, not writing it");
    return;
  }
  uint8_t buf = (plan_is_on + '0');

  if (fwrite(&buf, 1, 1, f) < 1)
    perror("could not write to plan status file");

  printf("%s: saved plan on status %d\n", current_time(), plan_is_on);
  fclose(f);
}

void save_new_plan_to_file()
{
  FILE *f = fopen(PLAN_PERSISTENT_FILE, "w+");
  if (f == 0)
  {
    perror("could not open plan file for writing, not writing it");
    return;
  }

  int filesize = strlen(packet);
  if (fwrite(packet, 1, filesize, f) < filesize)
    perror("could not write to plan file");

  printf("%s: saved new plan to file\n", current_time());
  fclose(f);
}

void try_to_load_plan_from_file()
{ 
  FILE *f;
  struct stat filestat;
  int filesize;

  if (stat(PLANON_PERSISTENT_FILE, &filestat))
    printf("planon file not found, not reading plan status\n");
  else
  {
    filesize = filestat.st_size;
    f = fopen(PLANON_PERSISTENT_FILE, "r");
    if (f == 0)
      perror("could not open plan status file, not reading plan status");
    else
    {
      if (!fread(packet, 1, 1, f))
        perror("could not read from plan status file");
      else
      {
        plan_is_on = (packet[0] - '0');
	printf("read from plan status file: plan_is_on=%d\n", plan_is_on);
      }
      fclose(f);
    }
  }

  if (stat(PLAN_PERSISTENT_FILE, &filestat))
  {
    printf("plan file not found, not loading plan\n");
    return;
  }
  filesize = filestat.st_size;
  if (filesize >= max_packet_size)
  {
    printf("plan file too large %d, max=%d\n", filesize, max_packet_size - 1);
    return;
  }
  
  f = fopen(PLAN_PERSISTENT_FILE, "r");
  if (f == 0) 
  {
    perror("could not open plan file");
    return;
  }
  if (fread(packet, 1, filesize, f) < filesize)
  {
     perror("could not read plan file");
     fclose(f);
     return;
  }
  packet[filesize] = 0;
  fclose(f);

  printf("found stored plan, loading it...\n");
  set_new_plan(packet);
}

int main(int argc , char *argv[])
{
  try_to_load_plan_from_file();

  struct sockaddr_in server;
  while (1)
  {
    while (!init_hw(0)) { printf("."); fflush(stdout); }

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
    	perror("Could not create socket");
    }
    	
    server.sin_addr.s_addr = inet_addr("158.195.89.120");
    server.sin_family = AF_INET;
    server.sin_port = htons( 9877 );

    if (connect(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
    	printf("connect error\n");
    	fflush(stdout);
	sleep(5);
	continue;
    }
    
    printf("sending login packet...\n");
    fflush(stdout);
    if (send_packet(login_request_packet, strlen(login_request_packet))) 
    {
        close(socket_desc);
	continue;
    }

    printf("sent, waiting for confirmation...\n");
    fflush(stdout);
    
    if (recv_packet()) 
    {
      close(socket_desc);
      continue;
    }
   
    if (strcmp(packet, login_response_packet) != 0)
    {
        printf("confirmation unrecognized\n");
        fflush(stdout);
	close(socket_desc);
        continue;
    }
    
    printf("handshake ok\n");
    fflush(stdout);

    connected = 1;
    send_beep(3);

    start_agenda_thread();
    start_button_thread();

    while (connected)
    { 
       if (recv_packet()) break;
       process_packet();
    }

    sleep(1);
    close_hw(0);
    close(socket_desc);
  }
  return 0;
}

