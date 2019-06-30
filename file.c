#include "file.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

const int block_size = 16;
const int tree_size = 256;
const int block_file_size = 12;
const int max_size_tree = 3;

int min(int a, int b) {
    if(a < b) {
        return a;
    }
    return b;
}

int small_pow(int a, int b) {
    if (b == 0) return 1;
    int res = a;
    for(int i = 1; i < b; ++i) {
        res *= a;
    }
    return res;
}

int str_to_blocks(char* buff){
    int blocks = strlen(buff) / block_size;
    if(strlen(buff) % block_size != 0){
        blocks++;
    }
    return blocks;
}

void DestructorFile(struct file* sample, int* pos) {
    AppendFreeRanges(*pos);
    free(sample);
}

void DeleteTree(struct file** files, int size) {
    for(int i = 0; i < size; ++i) {
        DestructorFile(files[i], &files[i]->start_pos_content);
    }
}

void DeleteLevelTree(struct block1024* parent) {
    if (parent->level == 1) {
        DeleteTree(parent->files, parent->sizeb);
        free(parent->files);
    }
    else{
        for (int i = 0; i < parent->sizec; ++i) 
        {
           DeleteLevelTree(parent->children[i]);
        }
    }
    free(parent);
}

void DestructorFileTree(struct fileTree* sample) {
    int size = min(block_file_size, sample->size);

    DeleteTree(sample->files, size);
    free(sample->files);

    if (sample->size > block_file_size) {
        for (int i = 0; i < sample->size - block_file_size; ++i) {
            DeleteLevelTree(sample->block_tree[i]);
        }
    }

    DestructorFile(sample->file_name, &sample->file_name->start_pos_name);
    free(sample);
}

struct file* WriteName(char* block) {//5
    FILE* save_file;
    int pos = ClosestPos();
    if (pos == -1) {
        save_file = fopen("save_file.txt", "a");
        pos = ftell(save_file);
    } else {
        save_file = fopen("save_file.txt", "r+");
    }

    if (save_file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    struct file* res = (struct file*)malloc(sizeof(struct file));
    res->start_pos_content = pos;
    res->start_pos_name = pos;
    fseek(save_file, pos, SEEK_SET);
    fwrite(block, block_size, 1, save_file);
    fclose(save_file);    
    return res;
}

struct file* WriteBlock(char* block, int name) {//5
    FILE* save_file;
    int pos = ClosestPos();
    if (pos == -1) {
        save_file = fopen("save_file.txt", "a");
        pos = ftell(save_file);
    } else {
        save_file = fopen("save_file.txt", "r+");
    }

    if (save_file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    struct file* res = (struct file*)malloc(sizeof(struct file));
    res->start_pos_content = pos; //1
    res->start_pos_name = name;//5
    fseek(save_file, pos, SEEK_SET);
    fwrite(block, block_size, 1, save_file);
    fclose(save_file);
    
    return res;
}

void WriteTree(struct file*** files, char* buff, int name) {
    int blocks = str_to_blocks(buff);
    *files = (struct file **) malloc(min(blocks, tree_size) * sizeof(struct file *));

    for (int i = 0; i < min(blocks, tree_size); ++i) {
        //int tmp; //1
        //(*files)[i] = WriteBlock(i*block_size + buff, &tmp); //1
        //(*files)[i]->start_pos_content = tmp; //1
        (*files)[i] = WriteBlock(i*block_size + buff, name); //1,5
    }
}

void WriteLevelTree(int level, char* buff, struct block1024* parent, int name) {//5
    parent->level = level;
    int blocks = str_to_blocks(buff);
    int size = blocks / small_pow(tree_size, level - 1);
    if (blocks % small_pow(tree_size, level - 1) != 0) {
        ++size;
    }
    size = min(tree_size, size);
    if(level == 1){
        parent->sizec = 0;
        parent->sizeb = size;
        parent->children = NULL;
        WriteTree(&parent->files, buff, name);//5
        return;
    }
    parent->sizec = size;//3 else{//3}
    parent->sizeb = 0;//3
    parent->files = NULL; //2//3
    parent->children = (struct block1024**)malloc(parent->sizec*sizeof(struct block1024*));
    for (int i = 0; i < parent->sizec; ++i) {
        parent->children[i] = (struct block1024*)malloc(sizeof(struct block1024));
        //printf("%i %i %i\n", level, small_pow(tree_size, level - 1) * i * block_size, i);
        WriteLevelTree(level - 1, buff + small_pow(tree_size, level - 1) * i * block_size,
                       parent->children[i], name);//5
    }
}

struct fileTree* Write(char* content, char* name) {
    struct fileTree* res = (struct fileTree*)malloc(sizeof(struct fileTree));
   
    int blocks = str_to_blocks(content);

    if(strlen(name) > block_size) {
        printf("Error! File name should be less then %d", block_size);
        return NULL;
    }

    res->file_name = WriteName(name);
    res->size = min(block_file_size, blocks);
    res->files = (struct file**)malloc(res->size*sizeof(struct file*));
    int nameI = (res->file_name)->start_pos_name;

    //int tmp = res->size;//7
    //WriteTree(&res->files, content, nameI);//7
    for(int i = 0; i < res->size; i++)//7
    {
        res->files[i] = WriteBlock(content + i*block_size, nameI);//7
    }
    blocks -= res->size;

    if(blocks <= 0) {
        res->block_tree = NULL;
        return res;
    }

    res->block_tree = (struct block1024**)malloc(max_size_tree*sizeof(struct block1024*));
    int summary_size = res->size;

    for (int i = 1; i <= max_size_tree; ++i) {
        res->block_tree[i - 1] = (struct block1024*)malloc(sizeof(struct block1024));
        WriteLevelTree(i, content + summary_size*block_size, res->block_tree[i - 1], nameI);
        blocks -= small_pow(tree_size, i);
        if (blocks <= 0) {
            res->size += i;
            return res;
        }
        summary_size += small_pow(tree_size, i);
    }

    assert(0);
    return NULL;
}

char* ReadBlock(int* pos) {
    char* res = (char*)malloc(block_size);
    FILE* save_file = fopen("save_file.txt", "r");
    fseek(save_file, *pos, SEEK_SET);
    fread(res, sizeof(char), block_size, save_file);
    fclose(save_file);
    res[block_size] = '\0';
    return res;
}


char* GetName(struct file* file) {
    return ReadBlock(&file->start_pos_name);
}

char* Concatenate(char* a, char* b) {
    char* res = (char *)malloc(1 + strlen(a)+ strlen(b));
    strcpy(res, a);
    strcat(res, b);
    free(a);
    free(b);

    return res;
}

char* ReadTree(struct file** files, int size) {
    char* res = NULL;
    for(int i = 0; i < size; ++i) {
        if(res == NULL) {
            res = ReadBlock(&files[i]->start_pos_content);
        } else {
            res = Concatenate(res, ReadBlock(&files[i]->start_pos_content));
        }
    }
    return res;
}

char* ReadLevelTree(struct block1024* parent) {
    char* res = NULL;
    if(parent->level == 1){
        res = ReadTree(parent->files, parent->sizeb); //8
        //if (res == NULL) {
        //    res = ReadTree(parent->files, parent->sizeb);
        //} else {
        //    res = Concatenate(res, ReadTree(parent->files, parent->sizeb));
        //}
    }
    else{
        for (int i = 0; i < parent->sizec; ++i) {
            if (res == NULL) {
                res = ReadLevelTree(parent->children[i]);
            } else {
                res = Concatenate(res, ReadLevelTree(parent->children[i]));
            } 
        }
    }
    return res;
}

char* Read(struct fileTree* file) {
    int size = min(block_file_size, file->size);
    char* res = ReadTree(file->files, size);

    if (file->size > block_file_size) {
        for (int i = 1; i <= file->size - block_file_size; ++i) {
            res = Concatenate(res, ReadLevelTree(file->block_tree[i - 1]));
        }
    }

    return res;
}
struct fileTree* WriteDir(char* dir_name) {
    if(strlen(dir_name) > block_size) {
        printf("Error! File name should be less then %d", block_size);
        return NULL;
    }

    struct fileTree* res = (struct fileTree*)malloc(sizeof(struct fileTree));
    res->file_name = WriteName(dir_name);
    res->size = 0;
    res->files = NULL;
    res->block_tree = NULL;
    return res;
}

