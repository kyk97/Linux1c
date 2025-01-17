/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2021 kyk_97 <kyk_97@mail.ru>
 * 
 * server is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * server is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include "string.h"

const int sizeI = 100;
const int sizeB = 1000;
const int block_size = 16;
const int inode_size = 256;
const int block_in_inode = 14;
const int free_inode_count = sizeI;
const int free_block_count = sizeB;
const int superblock_size = 10; 
const int block_in_block1024 = 1024;
char magic_number[16] = "1123581321345589";


const int command_size = 8;
const int first_arg_size = 8;
const int second_arg_size = 1024;
const int input_size = 1048;

char* Concatenate(char* a, char* b) {
    char* res = (char *)malloc(1 + strlen(a)+ strlen(b));
    strcpy(res, a);
    strcat(res, b);
    free(a);
    free(b);

    return res;
}

char* ReadBlockStr(int pos) {
    char* res = (char*)malloc(block_size);
    FILE* save_file = fopen("save_file.txt", "r");
    fseek(save_file, pos, SEEK_SET);
    fread(res, sizeof(char), block_size, save_file);
    fclose(save_file);
    res[block_size] = '\0';
    return res;
}

long int ReadBlockLInt(int pos){
	char * str = ReadBlockStr (pos);
	long int i = strtol(str, NULL, 10);
	free(str);
	return i;
}

int WriteBlockStr(char* block, int pos) {
    FILE* save_file;
    save_file = fopen("save_file.txt", "r+");

    if (save_file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    if (pos == -1) {
		fseek(save_file, 0, SEEK_END);		
        pos = ftell(save_file);			
    }

	if (pos >= superblock_size * block_size + sizeI + sizeB + sizeI * inode_size){
		int number_block = (pos - superblock_size * block_size - sizeI - sizeB - 
		sizeI * inode_size) / block_size;
		if (number_block > sizeB){
			printf("pos %d\n", pos);
			printf("Number_block %d\n", number_block);
			printf("Number_block out of sizeB");
			exit(1);
		}
		int pos_bool_block = number_block + superblock_size * block_size + sizeI;
    	fseek(save_file, pos_bool_block, SEEK_SET);
    	fwrite("1", 1, 1, save_file);
		
    	fseek(save_file, 6 * block_size, SEEK_SET);		
    	char* tmp = (char*)malloc(block_size);
    	fread(tmp, sizeof(char), block_size, save_file);
		long int free_blocks = strtol(tmp, NULL, 10);
		free(tmp);	
    	char* res = (char*)malloc(block_size + 1);		
		int cx = snprintf ( res, 17, "%016ld", free_blocks - 1); //block_size
		if (cx<0 && cx>=16){
			printf("Error in ReadBlockStr");
			exit(1);
		}
    	fseek(save_file, 6 * block_size, SEEK_SET);				
    	fwrite(res, block_size, 1, save_file);
		free(res);
	}
	
    fseek(save_file, pos, SEEK_SET);
    fwrite(block, block_size, 1, save_file);
    fclose(save_file);    
    return pos;
}

int WriteBlockInt(int pos, int i) {
    char out[16] =""; //block_size
    int cx;
    cx = snprintf ( out, 17, "%016d", i);//block_size
	if (cx<0 && cx>=16){
		printf("Error in WriteBlockInt");
		exit(1);
	}
    return WriteBlockStr (out, pos);
}

int FindPosBlockInBlock1024(int pos, int number_block){
	return pos + number_block * block_size;
}

int FindPosBlockInBlock1024InBlock1024(int pos, int number_block){
	pos = ReadBlockLInt (pos + (number_block / block_in_block1024)*block_size);

	number_block %= block_in_block1024;
	return FindPosBlockInBlock1024(pos, number_block);
}


int FindPosBlockInInode (int pos_inode, int number_block){
	int size = ReadBlockLInt (pos_inode);
	if (number_block > size){
		printf("Error find block! Array index out of size inode\n");
		exit(1);
	}
	if (number_block < block_in_inode - 2){
		return pos_inode + (number_block + 2) * block_size;
	}else if (number_block < block_in_block1024 + block_in_inode - 2){
		int pos = ReadBlockLInt(pos_inode + block_in_inode * block_size);
		return FindPosBlockInBlock1024(pos, number_block - (block_in_inode - 2));
	}
	else{
		if (number_block < block_in_block1024 * block_in_block1024 +
	        block_in_block1024 + block_in_inode - 2){
		number_block -= (block_in_inode - 2) + block_in_block1024;
		int pos = ReadBlockLInt(pos_inode + (block_in_inode + 1) * block_size);		
		return FindPosBlockInBlock1024InBlock1024(pos, number_block);
		}
		else{
			printf("Error find block! Array index out of max size inode\n");
			exit(1);
		}
	}
	printf("Error\n");
	return -1;
}

char* ReadBlockInInode (int pos_inode, int number_block){
	return ReadBlockStr(FindPosBlockInInode (pos_inode, number_block));
}




char* ReadFileInInode (int pos_inode){
	int size = ReadBlockLInt (pos_inode);
	int is_file = ReadBlockLInt (pos_inode + block_size);
	if (is_file != 1){
		printf("pos_inode %d\n", pos_inode);
		printf("is_file %d\n", is_file);
		printf("size %d\n", size);
		printf("Error read file! This is catalog\n");
		exit(1);		
	}
	char* res = NULL;
	for (int i = 0; i < size; i++){
        if(res == NULL) {
            res = ReadBlockInInode (pos_inode, i);
        } else {
            res = Concatenate(res, ReadBlockInInode (pos_inode, i));
        }
	}
	return res;
}

int StrToBlocks(char* buff){
    int blocks = strlen(buff) / block_size;
    if(strlen(buff) % block_size != 0){
        blocks++;
    }
    return blocks;
}

int min(int a, int b) {
    if(a < b) {
        return a;
    }
    return b;
}
int PosInodeInNumperInode(int number_inode){
	return superblock_size*block_size + sizeI + sizeB + number_inode*inode_size;
}
void WriteFileInInode (char* buff, int pos_inode){
	int size = StrToBlocks (buff);
	int is_file = 1;
	WriteBlockInt (pos_inode, size);
	WriteBlockInt (pos_inode + block_size, is_file);
	
	for(int i = 2; i < min(block_in_inode, size + 2); i++){
		WriteBlockStr (buff + (i - 2) * block_size, pos_inode + i * block_size);		
	}
	if (size <= block_in_inode - 2){
		return;
	}
	else{
		size -= (block_in_inode - 2);
		buff += (block_in_inode - 2) * block_size;
		int pos = WriteBlockStr (buff, -1);
		WriteBlockInt (pos_inode + block_in_inode * block_size, pos);		
		for(int i = 1; i < min(block_in_block1024, size); i++){
			WriteBlockStr (buff + i * block_size, -1);
		}
		if (size < block_in_block1024){
			return;
		}
		else{
			buff += block_in_block1024 * block_size;//  запись блока с сылками на блоки
			size -= block_in_block1024;
			char temp[16] = {0};//block_size
			int pos_b1024 = WriteBlockStr (temp, -1);
			WriteBlockInt (pos_inode + (block_in_inode + 1) * block_size, pos_b1024);
			for(int i = 1; i < block_in_block1024; i++){
				WriteBlockStr (temp, -1);
			}
			if (size > block_in_block1024 * block_in_block1024){
				printf("Error - file out of range (10+1024+1024*1024)*16 sympols");
				exit(1);
			}
			int n = size / block_in_block1024;
			for(int i = 0; i < n; i++){
				int pos = WriteBlockStr (buff, -1);
				WriteBlockInt (pos_b1024 + i * block_size, pos);				
				for(int j = 1; j < block_in_block1024; j++){
					WriteBlockStr (buff + j * block_size, -1);
				}
				buff += block_in_block1024 * block_size;
				size -= block_in_block1024;
			}
			if (size == 0) return;
			int pos = WriteBlockStr (buff, -1);
			WriteBlockInt (pos_b1024 + n * block_size, pos);				
			for(int j = 1; j < size; j++){
				WriteBlockStr (buff + j * block_size, -1);
			}
			
		}

	} 
}

char* GetText(int N){ //генерация текста для теста
    int i;
    char* res = (char*)malloc(N * sizeof(char));
    srand(time(NULL));
    for (i = 0; i < N - 1; i++){
        res[i] = 'a' + rand()%26; 
    }
    res[i] = '\0';
    return res;
}

char* GetCatalog(int pos_inode, char* name){
	char res[9];
	int cx = snprintf ( res, 9, "%08d", pos_inode); //block_size/2
	if (cx<0 && cx>=8){//blcok_size/2
		printf("Error in GetCatalog");
		exit(1);
	}
	char* ans= (char *)malloc(16 * sizeof(char)); //block_size
	strncpy(ans, res, 8);//block_size/2
	strncat(ans, name, 8);
	//printf("%s\n", ans);
	return ans;
}

int GetPosInCatalog(char *buff){
	char* tmp = (char *)malloc(8 * sizeof(char));
	strncpy(tmp, buff, 8);
	int ans = strtol(tmp, NULL, 10);
	free(tmp);
	return ans;
}

char* GetNameInCatalog(char *buff){
	char* tmp = (char *)malloc(8 * sizeof(char)); //block_size
	tmp = strncpy(tmp, buff + 8, 8);
	return tmp;
}

void CreateCatalogInode(char* parent, char* mi){// parent and mi - catalog
	int pos = GetPosInCatalog (mi);
	WriteBlockInt (pos, 2);
	WriteBlockInt (pos + block_size, 0);
	WriteBlockStr (parent, pos + 2 * block_size);
	WriteBlockStr (mi, pos + 3 * block_size);
}


long int GetFreeInode(){
	FILE* save_file;
    save_file = fopen("save_file.txt", "r+");
    fseek(save_file, 5 * block_size, SEEK_SET);		
    char* tmp = (char*)malloc(block_size);
    fread(tmp, sizeof(char), block_size, save_file);
	long int free_inodes = strtol(tmp, NULL, 10);
	free(tmp);	
    char* res = (char*)malloc(block_size + 1);		
	int cx = snprintf ( res, 17, "%016ld", free_inodes - 1); //block_size
	if (cx<0 && cx>=16){
		printf("Error in ReadBlockStr");
		exit(1);
	}		
    fseek(save_file, 5 * block_size, SEEK_SET);				
    fwrite(res, block_size, 1, save_file);
	free(res);
	int pos_number_inode = superblock_size * block_size + sizeI - free_inodes;		
    fseek(save_file, pos_number_inode, SEEK_SET);				
    fwrite("1", 1, 1, save_file);	
	int pos_inode = superblock_size * block_size + sizeI + sizeB + 
		(sizeI - free_inodes) * inode_size;
	return pos_inode; 
}

void AddCatalogInCatalogInode(int pos_inode, char* catalog){
	if (ReadBlockLInt (pos_inode + block_size) == 1){
		printf("Add catalog in file! Catalog can't add\n");
		return;
	}
	int size = ReadBlockLInt (pos_inode);
	WriteBlockInt (pos_inode, size + 1);
	if (size < block_in_inode - 2){
		WriteBlockStr(catalog, pos_inode + (size + 2) * block_size);
	}
	else{
		int pos = ReadBlockLInt (pos_inode + block_in_inode*block_size);
		size -= block_in_inode - 2;
		if (size > 1024){
			printf("Inode catalog decrease more then 1036 catlaog! Catalog can't add\n");
			return;
		}
		else{
			WriteBlockStr(catalog, pos + size * block_size);			
		}
	}
}

int FindNumberCatalogInCatalogInode(int pos_inode, char* name, int is_file){//name catalog
	if (ReadBlockLInt (pos_inode + block_size) == 1){
		printf("Find catalog in file! Catalog can't find\n");
		return -1;
	}
	int size = ReadBlockLInt (pos_inode);
	for (int i = 2; i < size; i++){
		char* catalog = ReadBlockInInode (pos_inode, i);
		char* catalog_name = GetNameInCatalog(catalog);
		int catalog_pos = GetPosInCatalog (catalog);
		int is_file_in_pos_inode = ReadBlockLInt (catalog_pos + block_size); 
		if (strcmp(name, catalog_name) == 0 && is_file == is_file_in_pos_inode){			
			free(catalog);
			free(catalog_name);
			return i;
		}
		free(catalog);
		free(catalog_name);
	}
	return -1;
}

void DeleteCatalogInCatalogInode(int pos_inode, char * name, int is_file, int sock){ //name catalog
	int number_catalog = FindNumberCatalogInCatalogInode(pos_inode, name, is_file);
	if (number_catalog == -1){
		if (is_file) send_answer(sock, "Don't find file %s.text", name); 
		else send_answer(sock, "Don't find catalog %s", name); 
		return;
	}
	int size = ReadBlockLInt (pos_inode);
	char* catalog = ReadBlockInInode (pos_inode, size - 1);
	if (number_catalog < block_in_inode - 2){
		WriteBlockStr(catalog, pos_inode + (number_catalog + 2) * block_size);	
	}
	else{
		int pos = ReadBlockLInt (pos_inode + block_in_inode*block_size);
		number_catalog -= block_in_inode - 2;
		if (number_catalog > 1024){
			send_answer(sock, "Inode catalog decrease more then 1036 catlaog! Catalog can't delete");			
		}
		else{
			WriteBlockStr(catalog, pos + number_catalog * block_size);			
		}
	}
	free (catalog);
	size -= 1;
	WriteBlockInt (pos_inode, size);
	return;	
}

char* LsCatalog(int pos_inode){
	
	char *ans = (char*)malloc(sizeof(char) * 1);
	ans[0] = ' ';
	if (ReadBlockLInt (pos_inode + block_size) == 1){
		strcat(ans, "Ls catalog in file! Catalog can't ls\n");
		return ans;
	}
	int size = ReadBlockLInt (pos_inode);
	for (int i = 2; i < size; i++){
		char* catalog = ReadBlockInInode (pos_inode, i);
		char* catalog_name = GetNameInCatalog(catalog);
		int pos = GetPosInCatalog (catalog);
		if (ReadBlockLInt (pos + block_size) == 1){
			strcat(catalog_name, ".text");
		}
		strcat(catalog_name, "    ");
		ans = Concatenate(ans, catalog);		
		free(catalog_name);
	}
	return ans;
}

void Cat(int pos_catalog_inode, char* name, int sock){//name without .text
	int number = FindNumberCatalogInCatalogInode(pos_catalog_inode, name, 1);
	if (number == -1){
        send_answer(sock, "Don't find file %s.text", name);
		return;
	}	
	char* catalog = ReadBlockInInode (pos_catalog_inode, number);		
	int pos_file_inode = GetPosInCatalog (catalog);
	free(catalog);
	char* file = ReadFileInInode (pos_file_inode);
	if (file != NULL) send_answer(sock, file);
	else send_answer(sock, "");
	free(file);
	return;
}



void init(){
    FILE* save_file;
    save_file = fopen("save_file.txt", "w");
    int pos = ftell(save_file);

    if (save_file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
	
    fseek(save_file, pos, SEEK_SET);
	fprintf(save_file,"%016d", sizeI);//block_size
	fprintf(save_file,"%016d", sizeB);//block_size
	fprintf(save_file,"%016d", block_size);//block_size
	fprintf(save_file,"%016d", inode_size);//block_size
	fprintf(save_file,"%016d", block_in_inode);//block_size
	fprintf(save_file,"%016d", free_inode_count);//block_size
	fprintf(save_file,"%016d", free_block_count);//block_size
	fprintf(save_file,"%016d", superblock_size);//block_size
	fprintf(save_file,"%16s", magic_number);//block_size
	fprintf(save_file,"%016d", block_in_block1024);//block_size
	fprintf(save_file,"%0100d", 0);//sizeI
	fprintf(save_file,"%01000d", 0);//sizeB
	fprintf(save_file,"%025600d", 0);//sizeI*inode_size
    fclose(save_file); 
	char * catalog0 = GetCatalog (GetFreeInode (), ".");
	CreateCatalogInode (catalog0, catalog0);
	free(catalog0);

    return;
}
void AddFileInCatalog(int pos_catalog_inode, char* name, char* buff){//buff - text
	int pos_inode = GetFreeInode();
	WriteFileInInode (buff, pos_inode);
	char * catalog = GetCatalog (pos_inode, name);
	AddCatalogInCatalogInode (pos_catalog_inode, catalog);
	free(catalog);
	return;	
}

char* GetWord(char* buff) {
    int i = 0;
    for(; i < strlen(buff); ++i) {
        if(buff[i] == ' ' || buff[i] == '\n') {
            break;
        }
    }
    char* ans = (char*)malloc(sizeof(char) * i);
    strncpy(ans, buff, i + 1);
    ans[i] = '\0';
    return ans;
}
char* GetName(char* t) {
    char label[5] = ".text";
	int size = strlen(t);
	if (size > 5 && strcmp(t + size - 5, label) == 0){
		size = size - 5;
	}
	char* ans = (char*)malloc(sizeof(char)*size);
	strncpy(ans, t, size);	
	ans[size] = '\0';
    return ans;
}

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

void Server_run() {
    daemon(0, 0);
    int sock, listener;
    struct sockaddr_in addr;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listener, (struct sockaddr *) &addr, sizeof(addr));
    listen(listener, 1);
    _Bool running = 1;

	init();
	int pos_catalog_inode = PosInodeInNumperInode (0);
	
    while (running == 1) {
        sock = accept(listener, NULL, NULL);

        char command[command_size + 1]; // +1 for terminating symbol
        char first_argument[first_arg_size + 1];
        char second_argument[second_arg_size + 1];
        char input_line[input_size + 1];

        while (recv(sock, input_line, input_size, 0) != 0) {
            int number_of_arguments = 
				safe_parse_input(sock, input_line, command_size, first_arg_size,
				                 second_arg_size, command, first_argument,
				                 second_argument);
			char* name = GetName(first_argument);
			int is_file = (strncmp(name, first_argument, strlen(first_argument)) != 0);
			int number1 = FindNumberCatalogInCatalogInode(pos_catalog_inode, 
			                                             name, 1);

            if(strcmp(command, "ls") == 0){
                char* output = LsCatalog(pos_catalog_inode);
                send_answer(sock, output);
                free(output);
            }
            else if (strcmp(command, "makedir") == 0){
				if(number_of_arguments == 1)
                    send_answer(sock, "Not sufficient arguments!");
                else {					
					if (is_file) {	
                    	send_answer(sock,"It is name f file!");
					}else{
						int number = FindNumberCatalogInCatalogInode(pos_catalog_inode, name, 0);
						if (number == -1){				
							int pos_inode = GetFreeInode ();
							char * catalog = GetCatalog (pos_inode, name);
							char * perent = ReadBlockStr (pos_catalog_inode + 3 * block_size);
							AddCatalogInCatalogInode (pos_catalog_inode, catalog);	
							CreateCatalogInode(perent, catalog);
							free(perent);		
							free(catalog);	
                    		send_answer(sock, "");		
						}
						else{
                    		send_answer(sock,"There is already a folder with this name");
						}
					}
				}                
            }
            else if (strcmp(command, "touch") == 0){
                if(number_of_arguments < 3)
                    send_answer(sock, "Not sufficient arguments!");
                else {									
					int number = FindNumberCatalogInCatalogInode(pos_catalog_inode, 
			                                             name, 1);
					if (number == -1){					
						AddFileInCatalog (pos_catalog_inode, name, second_argument);
					}
					else{
						char* catalog = ReadBlockInInode (pos_catalog_inode, number);		
						int pos_file_inode = GetPosInCatalog (catalog);
						free(catalog);
	            		WriteFileInInode (second_argument, pos_file_inode);
					}						
                    send_answer(sock, "");
				}
			}
            else if (strcmp(command, "rm") == 0){
                if(number_of_arguments == 1)
                    send_answer(sock, "Not sufficient arguments!");
                else {
					DeleteCatalogInCatalogInode(pos_catalog_inode, name, is_file, sock);	
                    send_answer(sock, "");
                }
            }
            else if (strcmp(command, "cat") == 0){
                if(number_of_arguments == 1)
                    send_answer(sock, "Not sufficient arguments!");
                else {
					if (!is_file){
                        send_answer(sock, "Unknown command");                		
					}
					else{
						Cat(pos_catalog_inode, name, sock);
					}
                }
            }

            else if (strcmp(command, "cd") == 0){
                if(number_of_arguments == 1)
                    send_answer(sock, "Not sufficient arguments!");
                else {
					if (is_file) {	
						send_answer(sock, "It is name f file!");
					}else{
						int number = FindNumberCatalogInCatalogInode(pos_catalog_inode, name, 0);
						if (number == -1){
							send_answer(sock, "Don't find catalog"); 
						}
						else{
							char* catalog = ReadBlockInInode (pos_catalog_inode, number);		
							int pos_inode = GetPosInCatalog (catalog);
							free(catalog);
            				pos_catalog_inode = pos_inode;
                   			send_answer(sock, "");
						}
					}	
                }
            }
			else if (strcmp(command, "..") == 0) {
				char* catalog = ReadBlockInInode (pos_catalog_inode, 0);
				if (pos_catalog_inode != PosInodeInNumperInode (0)){
			    	int pos_perent = GetPosInCatalog (catalog);			    
                	pos_catalog_inode = pos_perent;
				}
				free(catalog);
                send_answer(sock, "");
			}

            else if (strcmp(command, "help") == 0){
                send_answer(sock, "ls to list files in current directory\ncd $name$ to go to directory named $name$ in current directory\n"
                                  "makedir $name$ to create directory named $name$\ntouch $name$ $file text$ to create a file "
                                  "named $name$ with text of file $file text$\nrm $name$ to delete directory named $name$ "
                                  "and all files in it\nrm $name.text$ to delete file named $name.text$ from directory\n"
                                  "$..$ to go to perent of current directory\nexit to save and exit");
            }
            else if (strcmp(command, "exit") == 0) {
                send_answer(sock, "");
                break;
            }
            else if (strcmp(command, "stop") == 0){
                running = 0;
                send_answer(sock, "");
                break;
            }
            else {
                send_answer(sock, "Unknown command");
            }				
			free(name);
				
        }
    	close(sock);        
    }
}

int main(int argc, char* argv[]) {
    Server_run();
}