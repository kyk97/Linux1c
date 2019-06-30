#ifndef FS_EXT2_LIKE_INODE_H
#define FS_EXT2_LIKE_INODE_H

#include "file.h"

struct iNode {
    int size;
    struct iNode* parent;
    struct fileTree* file;
    struct iNode** children;
};

void DestructoriNode(struct iNode* sample);

int IsFolder(struct iNode* sample);

void PrintChildren(struct iNode* parent);

struct iNode* Find(struct iNode* parent, char *addr, int* param);

void ReallocChildrenArray(struct iNode* parent);

void AddExistedChild(struct iNode *parent, struct iNode* child);

struct iNode* AddNewChild(struct iNode *parent, struct fileTree *file);

void DeleteRecursively(struct iNode* sample);

struct iNode* DeleteChild(struct iNode* child);


#endif //FS_EXT2_LIKE_INODE_H
