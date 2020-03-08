#include <stdio.h>
#include <unistd.h>

#include "hw.h"

void test_full_scale(int channel)
{
	for (int i = 0; i < 255; i++)
        {
	  switch (channel)
	  {
	    case 1: set_color(1, i, 0, 0, 0, 0); break;
	    case 2: set_color(1, 0, i, 0, 0, 0); break;
	    case 3: set_color(1, 0, 0, i, 0, 0); break;
	    case 4: set_color(1, 0, 0, 0, i, 0); break;
          }
	  usleep(30000);
	  if (i % 5 == 0) printf("%d\n", i);
	}

	for (int i = 255; i >= 0; i--)
        {
	  switch (channel)
	  {
	    case 1: set_color(1, i, 0, 0, 0, 0); break;
	    case 2: set_color(1, 0, i, 0, 0, 0); break;
	    case 3: set_color(1, 0, 0, i, 0, 0); break;
	    case 4: set_color(1, 0, 0, 0, i, 0); break;
          }
	  usleep(30000);
	  if (i % 5 == 0) printf("%d\n", i);
	}
	printf("---\n");
}


int main()
{
	printf("connecting arduino...");
	while (!init_hw(0)) { printf("."); fflush(stdout); }

	test_full_scale(1);
	test_full_scale(2);
	test_full_scale(3);
	test_full_scale(4);

	close_hw(0);

	return 0;
}
