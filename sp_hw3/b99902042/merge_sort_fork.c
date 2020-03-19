#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "lib/merge_sort.h"
int countsize(){
    int fd=open("num.txt",O_RDWR);
    int a=lseek(fd,0,SEEK_END);
    a=a/10;
    return a;
}
int main(int argc,char* argv[])
{
    int max_procs=atoi(argv[1]);
    int len_chunk=atoi(argv[2]);
    int mer_degre=atoi(argv[3]);
    if(max_procs==0)    max_procs=1000;
    if(len_chunk==0)    len_chunk=100000;
    if(mer_degre==0)    mer_degre=25;
    int num=countsize();
    pid_t pid;
    int count_pro=0;
    int count_chu=0;
    char str[100];
    char *name[502];
    char name2[502][100];
    int i,j;
    for(i=0;i<num;i+=len_chunk){
        j=i+len_chunk;
        if(j>num) j=num;
        if((pid=fork())<0){
            wait(NULL);
            count_pro--;
            //printf("fork error\n");
        }
        else if(pid==0){
            sprintf(str,"merge_%d_%d",i,j);
            MP3_SORT(i,j,str);
            _exit(0);
        }
        else{
            count_pro++;
            if(count_pro>=max_procs){
                wait(NULL);
                count_pro--;
            }
            //wait();
        }
    }
    while(wait(NULL)!=-1);
    count_pro=0;
    int now=0;
    int unit=len_chunk;
    int index,end;
    int file;
    int filecount=0;
    if((num/mer_degre)!=0)  file=num/mer_degre+1;
    else    file=num/mer_degre;
    while(unit<num){
        while(now<num){
                if((pid=fork())<0){
                    wait(NULL);
                    count_pro--;
                    if(waitpid(pid,NULL,0)!=pid){
                        wait(NULL);
                        count_pro--;
                    }
                }
                else if(pid==0){
                    index=0;
                    if((now+mer_degre*unit)>num)    sprintf(name2[index],"merge_%d_%d",now,num);
                    else    sprintf(name2[index],"merge_%d_%d",now,now+mer_degre*unit);
                    name[index]=name2[index];
                    index++;
                    for(i=index;i<=mer_degre;i++){
                        if(now+unit>=num){
                            end=num;
                            sprintf(name2[index],"merge_%d_%d",now,end);
                            name[index]=name2[index];
                            now+=unit;
                            index++;
                            break;
                        }
                        else{
                            end=now+unit;
                            sprintf(name2[index],"merge_%d_%d",now,end);
                            name[index]=name2[index];
                            now+=unit;
                            index++;
                            //printf("innow1=%d\n",now);
                        }
                    }
                    //for(i=0;i<=mer_degre;i++){
                    //    printf("%d=%s\n",i,name[i]);
                    //}
                    //filecount++;
                    //if(filecount==file-1)
                    if(index>2) MP3_MERGE(index,name);
                    //if(strcmp(name[0],"merge_0_10000")==0){
                    //    for(int i=0;i<index;i++) printf("%s\n",name[i]);
                    //}
                    _exit(0);
                }
                else{
                    count_pro++;
                    if(count_pro>=max_procs){
                        wait(NULL);
                        count_pro--;
                    }
                    now+=mer_degre*unit;
                    //wait();
                }
        }
        now=0;
        unit*=mer_degre;
        while(wait(NULL)!=-1);
        if((file/mer_degre)!=0)  file=num/mer_degre+1;
        else    file=num/mer_degre;
    }
    return 0;
}
