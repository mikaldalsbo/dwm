/*
** Compile with:
** gcc -Wall -pedantic -std=c99 -lX11 status.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>

/*Template Import*/
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


static Display *dpy;

void setstatus(char *str) {
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

float getfreq(char *file) {
	FILE *fd;
	char *freq; 
	float ret;

	freq = malloc(10);
	fd = fopen(file, "r");
	if(fd == NULL) {
		fprintf(stderr, "Cannot open '%s' for reading.\n", file);
		exit(1);
	}

	fgets(freq, 10, fd);
	fclose(fd);

	ret = atof(freq)/1000000;
	free(freq);
	return ret;
}
char *loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *getdatetime() {
	char *buf;
	time_t result;
	struct tm *resulttm;

	if((buf = malloc(sizeof(char)*65)) == NULL) {
		fprintf(stderr, "Cannot allocate memory for buf.\n");
		exit(1);
	}
	result = time(NULL);
	resulttm = localtime(&result);
	if(resulttm == NULL) {
		fprintf(stderr, "Error getting localtime.\n");
		exit(1);
	}
	if(!strftime(buf, sizeof(char)*65-1, "%a %b %d %H:%M:%S", resulttm)) {
		fprintf(stderr, "strftime is 0.\n");
		exit(1);
	}
	
	return buf;
}

int getbattery() {
	FILE *fd;
	int energy_now, energy_full, voltage_now;

	fd = fopen("/sys/class/power_supply/BAT0/energy_now", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening energy_now.\n");
		return -1;
	}
	fscanf(fd, "%d", &energy_now);
	fclose(fd);


	fd = fopen("/sys/class/power_supply/BAT0/energy_full", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening energy_full.\n");
		return -1;
	}
	fscanf(fd, "%d", &energy_full);
	fclose(fd);


	fd = fopen("/sys/class/power_supply/BAT0/voltage_now", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening voltage_now.\n");
		return -1;
	}
	fscanf(fd, "%d", &voltage_now);
	fclose(fd);


	return ((float)energy_now * 1000 / (float)voltage_now) * 100 / ((float)energy_full * 1000 / (float)voltage_now);
}

int main(void) {
	char *status;
	float cpu0, cpu1, cpu2, cpu3;
	char *datetime;
	char *avgs;
	int bat0;


	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	if((status = malloc(200)) == NULL)
		exit(1);
	

	for (;;sleep(1)) {
		cpu0 = getfreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
		cpu1 = getfreq("/sys/devices/system/cpu/cpu1/cpufreq/scaling_cur_freq");
		cpu2 = getfreq("/sys/devices/system/cpu/cpu2/cpufreq/scaling_cur_freq");
		cpu3 = getfreq("/sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq");
		datetime = getdatetime();
		avgs = loadavg();
		bat0 = getbattery();
		snprintf(status, 200, "%s | %0.2f %0.2f %0.2f %0.2f | %d%% | %s", avgs, cpu0, cpu1, cpu2, cpu3, bat0, datetime);

		free(datetime);
		setstatus(status);
	}

	free(status);
	XCloseDisplay(dpy);

	return 0;
}

