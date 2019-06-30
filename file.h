#ifndef FS_EXT2_LIKE
#define FS_EXT2_LIKE

struct file {
    int start_pos_name;
    int start_pos_content;
};

struct block1024{
    int level;
    int sizeb;
    int sizec;
    struct file** files;
    struct block1024** children;
};

struct fileTree {
    int size;
    struct file* file_name;
    struct file** files;
    struct block1024** block_tree;
};

int min(int a, int b);
int small_pow(int a, int b);
int str_to_blocks(char* buff);
void DestructorFile(struct file* sample, int* pos);
void DeleteTree(struct file** files, int size);
void DeleteLevelTree(struct block1024* parent);
void DestructorFileTree(struct fileTree* sample);
struct file* WriteName(char* block);
struct file* WriteBlock(char* block, int name);
void WriteTree(struct file*** files, char* buff, int name);
void WriteLevelTree(int level, char* buff, struct block1024* parent, int name);
struct fileTree* Write(char* content, char* name);
char* ReadBlock(int* pos);
char* GetName(struct file* file);
char* ReadTree(struct file** files, int size);
char* ReadLevelTree(struct block1024* parent);
char* Read(struct fileTree* file);
struct fileTree* WriteDir(char* dir_name);


#endif //FS_EXT2_LIKE
