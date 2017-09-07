/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

struct node{
    char key;
    char value;
    struct node* next;
};

struct cmd{
    char command[3];
    int key;
    int value;
};

int count = 0;

struct cmd* parse(char* ch){
    struct cmd *tmp = malloc(sizeof(struct cmd));
    tmp->command[0] = ch[0];
    tmp->command[1] = ch[1];
    tmp->command[2] = ch[2];
    char str1[20], str2[20];
    int i = 4; int j=0;

    while(ch[i] != ' ' && i < strlen(ch)) {
        str1[i-4] = ch[i];
        i++;
    }
    tmp->key = atoi(str1);
    while(i < strlen(ch)) {
        str2[j] = ch[i];
        i++; j++;
    }
    tmp->value = atoi(str2);
    // printf("%s %d %d", tmp->command, tmp->key, tmp->value);
    return tmp;
}

int exists(struct cmd* cmnd, struct node* head){
    struct node* lhead = head;
    while(lhead!=NULL){
        if(lhead -> key == cmnd -> key) return 1;
        lhead=lhead->next;
    }
    return 0;
}

struct node* put(struct cmd* cmnd, struct node* head){
    if(!count){
        struct node *tmp = malloc(sizeof(struct node));
        tmp -> key = cmnd -> key; 
        tmp -> value = cmnd -> value;
        // printf("key:::%d, value:::%d\n", head -> key, head -> value);
        printf("%d\n", count); 
        /*head -> next = NULL;*/ count++;
        printf("HERE\n");printf("%d\n", count); return tmp;
    }
    else{
        struct node *tmp = malloc(sizeof(struct node));
        struct node* lhead = head;
        while(lhead->next!=NULL){lhead=lhead->next;}
        lhead -> next = tmp; 
        tmp -> next = NULL; 
        tmp -> key = cmnd -> key; 
        tmp -> value = cmnd -> value; 
        count++; return head;
    } 
}

struct node* del(struct cmd* cmnd, struct node* head){
    struct node* lhead = head;
    struct node* plhead = head; int cnt = 0;
    while(lhead != NULL){
        if(lhead -> key == cmnd -> key){
            if(cnt == 0){
                head = head -> next; 
                free(lhead); count--; 
                return head;
            }
            else{
                plhead -> next = lhead -> next;
                free(lhead); count--; 
                return head; 
            }
        }
        else{
            plhead = lhead;
            lhead = lhead -> next;
            cnt++;
        }
    }
    return head;
}

int get(struct cmd* cmnd, struct node* head){
    struct node* lhead = head;
    while(lhead!=NULL){
        if(lhead -> key == cmnd -> key) return lhead->value;
        lhead = lhead->next;
    }
}

void print(struct node* head){
    printf("List of all Key-Value pairs:\n");
    struct node* lhead = head;
    while(lhead!=NULL){
        printf("key->%d value->%d\n", lhead->key, lhead->value);
        lhead = lhead -> next;
    }
    printf("\n");
}


void error(char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    /* create socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    /* bind socket to this port number on this machine */

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
     
    /* listen for incoming connection requests */

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    /* accept a new request, create a newsockfd */

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
        error("ERROR on accept");

    struct node *head = NULL;
    char putstr[3], getstr[3], delstr[3], byestr[3]; 
    strcpy(putstr, "put"); strcpy(getstr, "get"); strcpy(delstr, "del"); strcpy(byestr, "Bye"); 
    // printf("%s %s %s %s\n", putstr, getstr, delstr, byestr);
    /* read message from client */
    while(1){
        bzero(buffer,256);
        n = read(newsockfd,buffer,255);

        if (n < 0) error("ERROR reading from socket");
        /* send reply to client */

        struct cmd* cmd1 = parse(&buffer);
        if(exists(cmd1, head)){
            if(!strcmp(cmd1->command, getstr)){
                int tmp = get(cmd1, head);
                char ttmp[256]; 
                sprintf(ttmp,"%d", tmp);
                n = write(newsockfd, &ttmp, strlen(ttmp));
                print(head);
                if (n < 0) error("ERROR writing to socket");
            }
            else if(!strcmp(cmd1->command, delstr)){
                head = del(cmd1, head);
                n = write(newsockfd,"OK", 2);
                print(head);
                if (n < 0) error("ERROR writing to socket");
            }
            else if(!strcmp(cmd1->command, putstr)){
                n = write(newsockfd,"Key already exists", 18);
                print(head);
                if (n < 0) error("ERROR writing to socket");
            }
            else if(!strcmp(cmd1->command, byestr)){
                n = write(newsockfd,"Goodbye", 13);
                if (n < 0) error("ERROR writing to socket");
                break;
            }
        }
        else{
            if(!strcmp(cmd1->command, getstr)){
                n = write(newsockfd,"Key not found", 13);
                print(head);
                if (n < 0) error("ERROR writing to socket");
            }
            if(!strcmp(cmd1->command, putstr)){
                head = put(cmd1, head);
                n = write(newsockfd,"OK", 2);
                print(head);
                if (n < 0) error("ERROR writing to socket");
            }
            if(!strcmp(cmd1->command, delstr)){
                n = write(newsockfd,"Key not found", 13);
                print(head);
                if (n < 0) error("ERROR writing to socket");                
            }
            else if(!strcmp(cmd1->command, byestr)){
                n = write(newsockfd,"Goodbye", 13);
                if (n < 0) error("ERROR writing to socket");
                break;
            }
        }     
    }
    return 0; 
}
