#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <time.h>
#include <pthread.h>

#include "hw.h"

#define max_packet_size 200000

#define MAX_PLAN_SIZE 200

static char *login_request_packet = "lampa calling, shake hand?";
static char *login_response_packet = "welcome lampa, hand shake!";

static int socket_desc;
static volatile int connected = 0;

static volatile int plan_is_on = 0;

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
  printf("loaded plan with %d steps\n", plan_len);
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

static char packet[max_packet_size];
static int len_pckt;

static int current_step;
static int current_interpolated_goal_step;
static uint8_t current_red, current_warm, current_cold1, current_cold2;

int recv_packet()
{
        unsigned char *pckt = (unsigned char *)packet;
        if (recv_data(packet, 4)) return 1;
        len_pckt = pckt[0] + (pckt[1] << 8);
        printf("packet[%d]\n", len_pckt);
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

static char *setcolor_packet = "setcolor";
static char *planon_packet = "planon";
static char *planoff_packet = "planoff";
static char *plantest_packet = "plantest";
static char *newplan_packet = "newplan:";
static char *bim_packet = "bim";
static char *bam_packet = "bam";

void process_packet()
{
  //printf("process packet %s\n", packet);
  if (strncmp(packet, setcolor_packet, strlen(setcolor_packet)) == 0)
  {
     int day, red, warm, cold1, cold2;
     sscanf(packet + strlen(setcolor_packet) + 1, "%d,%d,%d,%d,%d)", &day, &red, &warm, &cold1, &cold2);
     printf("setcolor(%d,%d,%d,%d,%d)\n", day, red, warm, cold1, cold2);
     fflush(stdout);
     set_color(day, red, warm, cold1, cold2);
  }
  else if (strncmp(packet, planon_packet, strlen(planon_packet)) == 0)
  {
    printf("planon\n");
    fflush(stdout);
    plan_is_on = 1;
  }
  else if (strncmp(packet, planoff_packet, strlen(planoff_packet)) == 0)
  {
    printf("planoff\n");
    fflush(stdout);
    plan_is_on = 0;
  }
  else if (strncmp(packet, plantest_packet, strlen(plantest_packet)) == 0)
  {
    printf("plantest\n");
    fflush(stdout);
  }
  else if (strncmp(packet, newplan_packet, strlen(newplan_packet)) == 0)
  {
    printf("newplan\n");
    fflush(stdout);
    set_new_plan(packet + strlen(newplan_packet));
  }
  else if (strncmp(packet, bim_packet, strlen(bim_packet)) == 0)
  {
    send_packet(bam_packet, strlen(bam_packet));
  }
  else { printf("unknown packet %s\n", packet); fflush(stdout); }
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
  set_color(1, plan_red[i], plan_warm[i], plan_cold1[i], plan_cold2[i]);
  char step_description[100];
  sprintf(step_description, "status step %02d [%02x,%02x,%02x,%02x,%d]", i, plan_red[i], plan_warm[i], plan_cold1[i], plan_cold2[i], plan_interpolate[i]);
  send_packet(step_description, strlen(step_description));
  printf("%s\n", step_description);
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

void *agenda_thread(void *args)
{
  adopt_new_plan = 0;
  while (connected)
  {
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
	    set_color(1, red_now, warm_now, cold1_now, cold2_now);
            printf("%02d [%02x,%02x,%02x,%02x]\n", current_interpolated_goal_step, red_now, warm_now, cold1_now, cold2_now);
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

int main(int argc , char *argv[])
{
	struct sockaddr_in server;
	
        while (!init_hw()) { printf("."); fflush(stdout); }

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		perror("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("158.195.89.120");
	server.sin_family = AF_INET;
	server.sin_port = htons( 9877 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		printf("connect error\n");
		fflush(stdout);
		return 1;
	}
        
        printf("sending login packet...\n");
  	fflush(stdout);
        if (send_packet(login_request_packet, strlen(login_request_packet))) return 1;

        printf("sent, waiting for confirmation...\n");
  	fflush(stdout);
        
        if (recv_packet()) return 1;
   
        if (strcmp(packet, login_response_packet) != 0)
        {
            printf("confirmation unrecognized\n");
  	    fflush(stdout);
            return 1;
        }
	
	printf("handshake ok\n");
	fflush(stdout);

        connected = 1;

        start_agenda_thread();

        while (connected)
        { 
           if (recv_packet()) return 1;
           process_packet();
        }

        sleep(1);
        close_hw();
        printf("quit\n");
	return 0;
}

