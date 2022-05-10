#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

#define MAXBUFSIZE 1024

float g_cpu_used;
typedef  struct occupy
{
    char name[20];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
} CPU_OCCUPY ;

void  cal_occupy (CPU_OCCUPY *o, CPU_OCCUPY *n){
    double od, nd;
    double id, sd;
    double scale;
    od = (double) (o->user + o->nice + o->system +o->idle);//第一次(用户+优先级+系统+空闲)的时间再赋给od
    nd = (double) (n->user + n->nice + n->system +n->idle);//第二次(用户+优先级+系统+空闲)的时间再赋给od
    scale = 100.0 / (float)(nd-od);       //100除强制转换(nd-od)之差为float类型再赋给scale这个变量
    id = (double) (n->user - o->user);    //用户第一次和第二次的时间之差再赋给id
    sd = (double) (n->system - o->system);//系统第一次和第二次的时间之差再赋给sd
    g_cpu_used = ((sd+id)*100.0)/(nd-od); //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
}

void  get_occupy (CPU_OCCUPY *o) {
    char buff[MAXBUFSIZE];
    FILE *fd = fopen ("/proc/stat", "r"); //这里只读取stat文件的第一行及cpu总信息，如需获取每核cpu的使用情况，请分析stat文件的接下来几行。
    fgets (buff, sizeof(buff), fd);
    sscanf (buff, "%s %u %u %u %u", o->name, &o->user, &o->nice,&o->system, &o->idle);
    fclose(fd);
}

double getCost()
{
    char result[1024] = {0};
    char buf[64] = {0};
    FILE *fp = NULL;

    if( (fp = popen("uptime", "r")) == NULL ) {
        printf("popen error!\n");
        return -1;
    }
    while (fgets(buf, sizeof(buf), fp)) {
        strcat(result, buf);
    }
    pclose(fp);
    char *idx = strrchr(result, ',');
    idx+=2;
    double res = atof(idx) * 100;
    printf("%.2f", res);
    return res;
}

int main() {
    getCost();
//    system("uptime");
//    CPU_OCCUPY ocpu,ncpu;
//    get_occupy(&ocpu);
//    sleep(1);
//    get_occupy(&ncpu);
//    cal_occupy(&ocpu, &ncpu);
//    printf("cpu used:%4.2f \n", g_cpu_used);
//    struct Transport transport;
//    printf("%d", sizeof(transport));
}

