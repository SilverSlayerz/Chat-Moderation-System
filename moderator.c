#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct{
    long mtype;
    int timestamp;
    int user;
    char mtext[256];
    int modifyingGroup;
    } Message;

void str_to_lower(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }
}

int count_violations(char filtered_words[][512], int filtered_count, char *msg_str) {
    int violation_count = 0;

    
    for (int i = 0; i < filtered_count; i++) {
        char *pos = msg_str;  
        while ((pos = strstr(pos, filtered_words[i])) != NULL) {  
            violation_count++;  
            pos += strlen(filtered_words[i]);  
        }
    }

    return violation_count;
}

int main(int argc,char* argv[]){
    if(argc!=2){
        printf("moderator.out didn't recieve correct number of arguments.");
        exit(1);
    }

    char input_file[50] = "testcase_";
    strcat(input_file, argv[1]);  
    strcat(input_file, "/input.txt");

    char filtered_file[50] = "testcase_";
    strcat(filtered_file, argv[1]);  
    strcat(filtered_file, "/filtered_words.txt");

    FILE *input_file_ptr = fopen(input_file, "r");
    FILE *filtered_file_ptr = fopen(filtered_file, "r");

    if (NULL == input_file_ptr) {
         printf("input file can't be opened by moderator \n");
           exit(1);
    }
    if (NULL == filtered_file_ptr) {
        printf("Filterd_words file can't be opened \n");
          exit(1);
   }

int moderator_key,violation_threshold;
fscanf(input_file_ptr, "%*d %*d %*d %d %d", &moderator_key, &violation_threshold);


int moderator_msgqid = msgget(moderator_key, 0666 | IPC_CREAT);


char filtered_words[50][512];
char word[512];
int filtered_count=0;



while((fscanf(filtered_file_ptr,"%s",word))==1){
    
    strcpy(filtered_words[filtered_count],word);
    str_to_lower(filtered_words[filtered_count]);
    filtered_count++;
}



int user_violations[30][50];
for(int i=0;i<30;i++){
    for(int j=0;j<50;j++){
        user_violations[i][j]=0;
    }
}

Message msg;


while (1) {  
   
   
if (msgrcv(moderator_msgqid, &msg, sizeof(msg) - sizeof(long), 95, 0) == -1) {
    perror("msgrcv from groups failed");
    exit(1);
} 

    if(msg.user==95){
        break;
    }
    str_to_lower(msg.mtext);
    

    

    int violations=count_violations(filtered_words,filtered_count,msg.mtext);
    
    user_violations[msg.modifyingGroup][msg.user]+=violations;
    
    msg.mtype=msg.modifyingGroup+1;
    if(user_violations[msg.modifyingGroup][msg.user]>=violation_threshold){
        msg.modifyingGroup=1;
        printf("User %d from group %ld has been removed due to %d violations @ time: %d.\n",msg.user,msg.mtype-1,user_violations[msg.mtype-1][msg.user],msg.timestamp);
    }else{
        msg.modifyingGroup=0;
    }
       
    msgsnd(moderator_msgqid, &msg, sizeof(msg) - sizeof(long), 0);
        
}



return 0;
}