/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2021 kyk_97 <kyk_97@mail.ru>
 * 
 * client is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * client is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <netinet/in.h>

const int name_size = 8;
const int command_size = 8;
const int first_arg_size = 8;
const int second_arg_size = 1024;
const int input_size = 1048;

int safe_parse_input(int sock, char* input, short first_size, short second_size, short third_size,
                     char* first_arg, char* second_arg, char* third_arg) {
    memset(first_arg, '\0', first_size + 1);
    memset(second_arg, '\0', second_size + 1);
    memset(third_arg, '\0', third_size + 1);

    short k = 0;
    short sizes[3] = {first_size, second_size, third_size};
    char* args[3] = {first_arg, second_arg, third_arg};
    int current_arg = 0;
    short j = 0;
    while ((k < strlen(input)) && (current_arg < 3) && (input[k] != '\n')) {
        while ((j < sizes[current_arg]) && (input[k] != ' ') && (k < strlen(input)) && (input[k] != '\n')) {
            args[current_arg][j] = input[k];
            j++;
            k++;
        }

        if ((j == sizes[current_arg]) && (input[k] != ' ') && (k < strlen(input)) && (input[k] != '\n')) {
            send_answer(sock, "Argument too long!");
            break;
        }
        j = 0;
        current_arg++;
        k++;
    }


    return current_arg;
}

void send_answer(int sock, char* answer){
    char buf[16];
    sprintf(buf, "%i", strlen(answer));
    send(sock, buf, 16, 0);
    send(sock, answer, strlen(answer), 0);
}

char* get_answer(int sock){
    char buf[16];
    recv(sock, buf, 16, 0);
    int size_of_answer;
    sscanf(buf, "%i", &size_of_answer);

    char *answer = malloc(sizeof(char) * (size_of_answer + 1));
    memset(answer, 0, (size_of_answer + 1));

    recv(sock, answer, (size_t)size_of_answer, 0);
    return answer;
}


void Client_run() {
    int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    char name[name_size];

    char command[command_size + 1]; // +1 for terminating symbol
    char first_argument[first_arg_size + 1];
    char second_argument[second_arg_size+ 1];
    char input_line[input_size + 1];


    while(fgets(input_line, input_size, stdin) != 0){
        send(sock, input_line, input_size, 0);
        safe_parse_input(sock, input_line, command_size, first_arg_size, second_arg_size,
                         command, first_argument, second_argument);

        char *answer = get_answer(sock);
        if (strlen(answer) > 0)
            printf("%s\n", answer);
        free(answer);

        if (strcmp(command, "exit") == 0)
            break;
        else if (strcmp(command, "stop") == 0)
                break;
    }
    close(sock);

}

int main() {
    Client_run();
}