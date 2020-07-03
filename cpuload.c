#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_CORES 1

static pthread_t threadId;

static float lastCpuload;

static inline float computeCpuLoad(int msInterval)
{
    int i;
    long double a[5][4], b[5][4], loadavg[5];
    FILE *fp;

    fp = fopen("/proc/stat","r");
    for(i=0;i<NUM_CORES;i++)
        fscanf(fp,"%*s %Lf %Lf %Lf %Lf %*s %*s %*s %*s %*s %*s",&a[i][0],&a[i][1],&a[i][2],&a[i][3]);
    fclose(fp);

    usleep(1000*msInterval);

    fp = fopen("/proc/stat","r");
    for(i=0;i<NUM_CORES;i++)
        fscanf(fp,"%*s %Lf %Lf %Lf %Lf %*s %*s %*s %*s %*s %*s",&b[i][0],&b[i][1],&b[i][2],&b[i][3]);
    fclose(fp);

    for(i=0;i<NUM_CORES;i++)
       loadavg[i] = ((b[i][0]+b[i][1]+b[i][2]) - (a[i][0]+a[i][1]+a[i][2])) / ((b[i][0]+b[i][1]+b[i][2]+b[i][3]) - (a[i][0]+a[i][1]+a[i][2]+a[i][3]));

    return (float)loadavg[0];
}

void* cpuLoadThread(void *params)
{
    fprintf(stderr, "%s() - Info: CPU utilization thread started...\n", __FUNCTION__);

    while(1)
    {
        lastCpuload = computeCpuLoad(2000);
    }
}

float getCpuLoad(void)
{
    return lastCpuload*100;
}

void startCpuLoadDetectionThread(void)
{
    pthread_create(&threadId, NULL, &cpuLoadThread, NULL);
}

void stopCpuLoadDetectionThread(void)
{
    pthread_cancel(threadId);
    fprintf(stderr, "%s() - Info: CPU utilization thread stopped\n", __FUNCTION__);
}
