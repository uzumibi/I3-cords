#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include "bandpass2.h"

int s;
void* audio_processing(void* arg) {

    char command[] = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    short int buffer[8192];
    short int processed_buffer[8192];
    size_t read_size;

    while((read_size = fread(buffer, 2, sizeof(buffer)/2, fp)) > 0) {
        bandpass(buffer, processed_buffer, sizeof(buffer)/2);

        send(s, processed_buffer, read_size*2, 0);

        short int recv_buf[8192];
        int recv_len = recv(s, recv_buf, sizeof(recv_buf), 0);
        if (recv_len == 0) {
            perror("recv");
            break;
        }
        write(1, recv_buf, recv_len);
    }
    if (fclose(fp) == -1) {
        perror("fclose failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    return NULL;
}

void* success_processing(void* arg) {
    short int buffer[8192];
    size_t read_size;

    FILE *fp2 = fopen("success.raw", "r");
    if (fp2 == NULL) {
        perror("fopen for success.raw failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    while ((read_size = fread(buffer, 2, sizeof(buffer) / 2, fp2)) > 0) {
        send(s, buffer, read_size * 2, 0);

        short int recv_buf[8192];
        int recv_len = recv(s, recv_buf, sizeof(recv_buf), 0);
        if (recv_len == 0) {
            perror("recv");
            break;
        }
        write(1, recv_buf, recv_len);
    }

    if (fclose(fp2) != 0) {
        perror("fclose failed");
        close(s);
        exit(EXIT_FAILURE);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if (argc == 2) {
        int port = atoi(argv[1]);

        int ss = socket(PF_INET, SOCK_STREAM, 0);
        if (ss == -1) {
            perror("socket");
            return 1;
        }
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("bind");
            return 1;
        }

        if (listen(ss, 10) == -1) {
            perror("listen");
            return 1;
        }

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        s = accept(ss, (struct sockaddr *)&client_addr, &len);
        if (s == -1) {
            perror("accept");
            return 1;
        }

        close(ss);
    } else if (argc == 3) {
        char *ip_addr = argv[1];
        int port = atoi(argv[2]);

        s = socket(PF_INET, SOCK_STREAM, 0);
        if (s == -1) {
            perror("socket");
            return 1;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if (inet_aton(ip_addr, &addr.sin_addr) == 0) {
            printf("Invalid IP address\n");
            return 1;
        }
        addr.sin_port = htons(port);

        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            perror("connect");
            close(s);
            return 1;
        }
    }
    pthread_t audio_thread, success_thread;
    pthread_create(&audio_thread, NULL, audio_processing, NULL);
    pthread_create(&success_thread, NULL, success_processing, NULL);

    pthread_join(success_thread, NULL);
    pthread_join(audio_thread, NULL);

    close(s);
    return 0;
}
