#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_NUMBER_OF_GROUPS 30

typedef struct {
    long mtype;
    int timestamp;
    int user;
    char mtext[256];
    int modifyingGroup;
} Message;

typedef struct {
    long mtype;
    int group_id;
} TerminationMessage;

typedef struct {
    char user_file[50];
    int pipe_fd;
    int groupuser_no;
    int validation_msgqid;
    int group_no;
} ThreadData;

void* user_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    FILE* file_ptr = fopen(data->user_file, "r");
    if (file_ptr == NULL) {
        printf("User file %s can't be opened\n", data->user_file);
        pthread_exit(NULL);
    }

    Message user_creation_msg = {2, 0, data->groupuser_no, "", data->group_no};
    msgsnd(data->validation_msgqid, &user_creation_msg, sizeof(user_creation_msg) - sizeof(long), 0);

    Message temp_msg;
    int timestamp;
    char msg[256];

    while (fscanf(file_ptr, "%d %s", &timestamp, msg) == 2) {
        temp_msg.timestamp = timestamp;
        temp_msg.user = data->groupuser_no;
        strcpy(temp_msg.mtext, msg);
        write(data->pipe_fd, &temp_msg, sizeof(Message));
    }
    
    fclose(file_ptr);
    close(data->pipe_fd);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        printf("Groups didn't receive the correct number of arguments.\n");
        exit(1);
    }

    char *ptr;
    int validation_key = strtol(argv[3], &ptr, 10);
    int app_key = strtol(argv[4], &ptr, 10);
    int moderator_key = strtol(argv[5], &ptr, 10);
    int violations = strtol(argv[6], &ptr, 10);

    char group_file[50] = "testcase_";
    strcat(group_file, argv[1]);
    strcat(group_file, "/");
    strcat(group_file, argv[2]);

    FILE *file_ptr = fopen(group_file, "r");
    if (file_ptr == NULL) {
        printf("Group file can't be opened \n");
        exit(1);
    }

    int group_no;
    sscanf(argv[2], "groups/group_%d.txt", &group_no);

    int users;
    fscanf(file_ptr, "%d", &users);
    char user_number[users][30];

    for (int i = 0; i < users; i++) {
        fscanf(file_ptr, "%s", user_number[i]);
    }
    fclose(file_ptr);

    int moderator_msgqid = msgget(moderator_key, 0666 | IPC_CREAT);
    int validation_msgqid = msgget(validation_key, 0666 | IPC_CREAT);
    int app_msgqid = msgget(app_key, 0666 | IPC_CREAT);
    
    Message group_creation_msg = {1, 0, 0, "", group_no};
    msgsnd(validation_msgqid, &group_creation_msg, sizeof(group_creation_msg) - sizeof(long), 0);

    int pipes[users][2];
    pthread_t threads[users];
    ThreadData thread_data[users];
    
    for (int i = 0; i < users; i++) {
        if (pipe(pipes[i]) == -1) {
            printf("Pipe creation failed\n");
            exit(1);
        }
    }
    
    for (int i = 0; i < users; i++) {
        thread_data[i].pipe_fd = pipes[i][1];
        thread_data[i].group_no = group_no;
        thread_data[i].validation_msgqid = validation_msgqid;
        sscanf(user_number[i], "users/user_%*d_%d.txt", &thread_data[i].groupuser_no);
        snprintf(thread_data[i].user_file, sizeof(thread_data[i].user_file), "testcase_%s/%s", argv[1], user_number[i]);
        
        pthread_create(&threads[i], NULL, user_thread, &thread_data[i]);
    }
    
    for (int i = 0; i < users; i++) {
        pthread_join(threads[i], NULL);
    }

    Message message_queue[5000];
    int msg_count = 0;
    int user_msg_count[50] = {0};
    int user_ban[50] = {0};

    for (int i = 0; i < users; i++) {
        close(pipes[i][1]);
        Message temp_msg;

        while (read(pipes[i][0], &temp_msg, sizeof(Message)) > 0) {
            user_msg_count[temp_msg.user]++;
            message_queue[msg_count] = temp_msg;
            msg_count++;
        }
        close(pipes[i][0]);
    }

    for (int i = 0; i < msg_count; i++) {
        for (int j = i + 1; j < msg_count; j++) {
            if (message_queue[i].timestamp > message_queue[j].timestamp) {
                Message temp_msg = message_queue[j];
                message_queue[j] = message_queue[i];
                message_queue[i] = temp_msg;
            }
        }
    }
    
     
    int violated_users=0;
    int i = 0;
    while (users >= 2) {
        if (user_ban[message_queue[i].user] == 1){
            printf("No more msg from User %d from group %d @ time: %d\n",message_queue[i].user,group_no,message_queue[i].timestamp);
        }
        if (user_ban[message_queue[i].user] != 1) {
            Message msg_to_moderator = {95, message_queue[i].timestamp, message_queue[i].user, "", group_no};
            strcpy(msg_to_moderator.mtext, message_queue[i].mtext);
            printf("Msg sent group: %d, user: %d, timestamp: %d\n",msg_to_moderator.modifyingGroup,msg_to_moderator.user,msg_to_moderator.timestamp);
            if (msgsnd(moderator_msgqid, &msg_to_moderator, sizeof(msg_to_moderator) - sizeof(long), 0) == -1) {
                perror("msgsnd to moderator failed");
                exit(1);
            }

            if (msgrcv(moderator_msgqid, &msg_to_moderator, sizeof(msg_to_moderator) - sizeof(long), group_no + 1, 0) == -1) {
                perror("msgrcv from moderator failed");
                exit(1);
            }
            printf("Msg recv group: %ld, user: %d, timestamp: %d\n",msg_to_moderator.mtype-1,msg_to_moderator.user,msg_to_moderator.timestamp);
            if(msg_to_moderator.user!=message_queue[i].user){
                printf("User numbers are not the same!\n");
            }
            if (msg_to_moderator.modifyingGroup == 1) {
                user_ban[message_queue[i].user] = 1;
                printf("Msg q User %d from group %d has been removed @ time: %d.\nMod q User %d from group %ld has been removed @ time: %d.\n",message_queue[i].user,group_no,message_queue[i].timestamp,msg_to_moderator.user,msg_to_moderator.mtype-1,msg_to_moderator.timestamp);
                violated_users++;
                printf("Group: %d users left: %d\n",group_no,users-1);
                users--;
            } else {
                user_msg_count[message_queue[i].user]--;
                if (user_msg_count[message_queue[i].user] == 0) {
                    printf("Group: %d users left: %d\n",group_no,users-1);
                    users--;
                }
            }
            
            Message msg_to_validation = {MAX_NUMBER_OF_GROUPS + group_no, message_queue[i].timestamp, message_queue[i].user, "", 0};
            
            strcpy(msg_to_validation.mtext, message_queue[i].mtext);
            msgsnd(validation_msgqid, &msg_to_validation, sizeof(msg_to_validation) - sizeof(long), 0);
        }
        i++;
    }
    printf("exited group: %d\n",group_no);
    Message termination_msg = {3, 0, violated_users, "", group_no};
    msgsnd(validation_msgqid, &termination_msg, sizeof(termination_msg) - sizeof(long), 0);

    TerminationMessage app_termination_msg = {1, group_no};
    msgsnd(app_msgqid, &app_termination_msg, sizeof(app_termination_msg) - sizeof(long), 0);
    
    return 0;
}