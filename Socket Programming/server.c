/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#define EPOLL_QUEUE_LEN 100
#define MAX_EVENTS 1000

struct node *head = NULL;
int count = 0;

void error(char *msg){
    perror(msg);
    exit(1);
}

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
        count++;
        return tmp;
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

static int make_socket_non_blocking (int sfd){
    int flags, s;
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1){
        perror ("fcntl");
        return -1;
    }
    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1){
          perror ("fcntl");
          return -1;
    }
    return 0;
}

int main(int argc, char *argv[]){
    int s, efd;
    struct epoll_event event;
    struct epoll_event *events;

    int sockfd, portno;
    struct sockaddr_in serv_addr;
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
    s = make_socket_non_blocking(sockfd);
    if(s < 0)
        error("ERROR socket could not be made non-blocking");

    listen(sockfd, 5);

    efd = epoll_create1(0);
    if(efd < 0)
      error ("ERROR creating epoll instance");

    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;

    s = epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &event);
    if (s == -1)
        error ("ERROR epoll_ctl");

    events = calloc(MAX_EVENTS, sizeof(event));

    while(1){
        int n, i;
        n = epoll_wait(efd, events, MAX_EVENTS, -1);
        if(n < 0)
           error("ERROR on epoll_wait");

        for(i=0; i<n; i++){
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))){
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
                // fprintf (stderr, "epoll error\n");
                continue;
            }
            else if (sockfd == events[i].data.fd){
                /*  We have a notification on the listening socket, which means one or more incoming connections. */
                while(1){
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof(in_addr);
                    infd = accept(sockfd, &in_addr, &in_len);
                    if (infd < 0){
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                            /* We have processed all incoming connections. */
                            break;
                        }
                        else{
                            perror ("accept");
                            break;
                        }
                    }

                    s = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0)
                        printf("Accepted connection on descriptor %d " "(host=%s, port=%s)\n", infd, hbuf, sbuf);

                    /* Make the incoming socket non-blocking and add it to the list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if(s < 0)
                        error("ERROR socket could not be made non-blocking");

                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                    if (s < 0)
                        error("epoll_ctl");
                }
                continue;
            }
            else{
                char putstr[3], getstr[3], delstr[3], byestr[3]; 
                strcpy(putstr, "put"); strcpy(getstr, "get"); strcpy(delstr, "del"); strcpy(byestr, "Bye");
                //Code per client:~
                while(1){
                    size_t count1;
                    char buffer[256];
                    int n1;
                    
                    /* read message from client */
                    count1 = read(events[i].data.fd, buffer, 255);
                    if(count1 == -1){
                        if (errno != EAGAIN){
                            perror ("read");
                        }
                        break;
                    }
                    else if(count1 == 0){
                      /* End of file. The remote has closed the
                         connection. */
                        break;
                    }
                    /* send reply to client */
                    struct cmd* cmd1 = parse(buffer);
                    // printf("%s, %d, %d\n", cmd1->command, cmd1->key, cmd1->value);
                    if(exists(cmd1, head)){
                        if(!strcmp(cmd1->command, getstr)){
                            int tmp = get(cmd1, head);
                            char ttmp[256]; 
                            sprintf(ttmp,"%d", tmp);
                            n1 = write(events[i].data.fd, &ttmp, strlen(ttmp));
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");
                        }
                        else if(!strcmp(cmd1->command, delstr)){
                            head = del(cmd1, head);
                            n1 = write(events[i].data.fd,"OK", 2);
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");
                        }
                        else if(!strcmp(cmd1->command, putstr)){
                            n1 = write(events[i].data.fd,"Key already exists", 18);
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");
                        }
                        else if(!strcmp(cmd1->command, byestr)){
                            n1 = write(events[i].data.fd,"Goodbye", 7);
                            if (n1 < 0) error("ERROR writing to socket");
                            close(events[i].data.fd); 
                            break;
                        }
                        else{
                            n1 = write(events[i].data.fd,"Command not found.", 18);
                            if (n1 < 0) error("ERROR writing to socket");
                            break;
                        }
                    }
                    else{
                        if(!strcmp(cmd1->command, getstr)){
                            n1 = write(events[i].data.fd,"Key not found", 13);
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");
                        }
                        else if(!strcmp(cmd1->command, putstr)){
                            head = put(cmd1, head);
                            n1 = write(events[i].data.fd,"OK", 2);
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");
                        }
                        else if(!strcmp(cmd1->command, delstr)){
                            n1 = write(events[i].data.fd,"Key not found", 13);
                            // print(head);
                            if (n1 < 0) error("ERROR writing to socket");                
                        }
                        else if(!strcmp(cmd1->command, byestr)){
                            n1 = write(events[i].data.fd,"Goodbye", 7);
                            close(events[i].data.fd); 
                            if (n1 < 0) error("ERROR writing to socket");
                            break;
                        }
                        else{
                            n1 = write(events[i].data.fd,"Command not found.", 18);
                            if (n1 < 0) error("ERROR writing to socket");
                            break;
                        }
                    }
                }
                continue;
            }
        }
    }
    free(events);
    close(sockfd);
    return 0; 
}
