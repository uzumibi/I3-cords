#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "bandpass2.h"
#include <string.h>
int main(int argc, char *argv[]){
    int s;

    if(argc == 2){
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
        bind(ss, (struct sockaddr *)&addr, sizeof(addr));

        listen(ss, 10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        s = accept(ss, (struct sockaddr *)&client_addr, &len);
    } else if(argc == 3) {
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

        int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
        if (ret != 0) {
            perror("connect");
            close(s);
            return 1;
        }
    }

    char command_rec[] = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
    FILE *fp = popen(command_rec, "r");
    if (fp == NULL) {
        perror("popen failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    short int buffer[1024];
    short int processed_buffer[1024+256];
    size_t read_size;

    FILE *fp2 = fopen("success.raw", "r");
    if (fp2 == NULL) {
        perror("fopen for success.raw failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    while ((read_size = fread(buffer, 2, sizeof(buffer) / 2, fp2)) > 0) {
        send(s, buffer, read_size * 2, 0);

        short int recv_buf[1024+256];
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
    char chat[256];
    char command;
    short int recv_buf[1024+256];
    FILE *fp_song;
    while(1) {
        command = ' ';
        FILE *fp_chat_log = fopen("chat_log.txt", "a");
        read_size = fread(buffer,2, sizeof(buffer)/2,fp);
        FILE *fp_chat_send = fopen("chat_send.txt", "r");
        if (fgets(chat, sizeof(chat), fp_chat_send)!= NULL && chat[0]>='a' && chat[0]<='z'){
            fputs(chat, fp_chat_log);
            fputs("\n", fp_chat_log);
        }
        bandpass(buffer, processed_buffer, sizeof(buffer)/2);
        for (int i = 0; i < 256; i++){
            processed_buffer[i+1024] = (short)chat[i];
        }
        FILE *fp_command = fopen("command.txt", "r");
        fread(&command, 1, 1, fp_command);
        if (command != 'c') {
            fp_song = fopen("song.raw", "r");
        }
        if (command=='c'){
            fread(&processed_buffer, 2, 1024, fp_song);
        }
        else if (command=='m'){
            memset(&processed_buffer, 0, sizeof(short)*1024);
        }
        send(s, processed_buffer,sizeof(short)*(1024+256), 0);
        memset(&chat, 0, 256);
        int recv_len = recv(s, recv_buf, sizeof(short)*(1024+256), 0);
        if (recv_len == 0) {
            perror("recv");
            break;
        }




        write(1, recv_buf, sizeof(short)*1024);
        for (int i= 0; i < 256; i++){
            chat[i] = (char)recv_buf[i+1024];
        }
        if (chat[0] >='a' && chat[0]<'z'){
            fputs(chat, fp_chat_log);
            fputs("\n", fp_chat_log);
        }
        memset(&chat, 0, 256);
        fclose(fp_chat_send);
        fp_chat_send = fopen("chat_send.txt", "w");
        fclose(fp_chat_send);
        fclose(fp_chat_log);
    }
    if (pclose(fp) == -1) {
        perror("pclose failed");
        close(s);
        exit(EXIT_FAILURE);
    }
    close(s);
    return 0;
}

// ./band_phone 10.100.205.123 50036 | play -t raw -c 1 -b 16 -e s -r 44100 -