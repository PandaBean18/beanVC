#ifndef FILE_NODE_H
#define FILE_NODE_H

#include "versionManager.h"
#include "lineNode.h"

class FileNode
{
    public:
    LineNode *lines;
    FileNode();
};

#endif