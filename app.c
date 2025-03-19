#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    long mtype;
    int group_id;
} TerminationMessage;

typedef struct{
    long mtype;
    int timestamp;
    int user;
    char mtext[256];
    int modifyingGroup;
    } Message;

int main(int argc, char* argv[]){

    if(argc!=2){
        printf("App.out didn't recieve correct number of arguments.");
        exit(1);
    }
    char input_file[50] = "testcase_";
    strcat(input_file, argv[1]);  
    strcat(input_file, "/input.txt");

    FILE *file_ptr = fopen(input_file, "r");

    if (NULL == file_ptr) {
         printf("file can't be opened \n");
           exit(1);
    }

    int groups, key1, key2, key3, violation;
    char group_files[30][50];

    fscanf(file_ptr, "%d %d %d %d %d", &groups, &key1, &key2, &key3, &violation);
    
    for (int i = 0; i < groups; i++) {
        fscanf(file_ptr, "%s", group_files[i]);
    }
    fclose(file_ptr);

    
    char key1_str[20];
    sprintf(key1_str, "%d", key1);

    char key2_str[20];
    sprintf(key2_str, "%d", key2);

    char key3_str[20];
    sprintf(key3_str, "%d", key3);

    char violation_str[20];
    sprintf(violation_str,"%d",violation);
    

    for(int i=0;i<groups;i++){
        pid_t pid=fork();
        if(pid==0){
            char *args[]={"./groups.out",argv[1],group_files[i],key1_str,key2_str,key3_str,violation_str,NULL};
            
            execv(args[0],args);
            exit(1);
        }else if(pid<0){
            printf("Fork failed.");
            exit(1);
        }
        

    }

    int groupqid = msgget(key2, 0666 | IPC_CREAT);
    if (groupqid == -1) {
        exit(1);
    }


    int active_groups = groups;
    printf("key in app:%d \n",key2);
    TerminationMessage msg;

    
    while (active_groups > 0) {
        if (msgrcv(groupqid, &msg, sizeof(msg) - sizeof(long), 0, 0) != -1) {
            printf("All users terminated. Exiting group process %d.\n", msg.group_id);
            active_groups--;
        }
    }

    
    int msgqid = msgget(key3, 0666 | IPC_CREAT);

    Message msg_to_moderator;
    msg_to_moderator.mtype=95;
    msg_to_moderator.user=95;
    msgsnd(msgqid, &msg_to_moderator, sizeof(msg) - sizeof(long), 0);

    
    return 0;
     
    
     
}