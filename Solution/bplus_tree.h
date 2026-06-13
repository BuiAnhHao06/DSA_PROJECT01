#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "bplus_tree_disk.h"

void readNode(FILE *fp, long offset, BPlusNode &node);

void writeNode(FILE *fp, long offset, const BPlusNode &node);

long allocateNode(FILE *fp, DBHeader &header);

void insertRecord(FILE *fp, DBHeader &header, int id, const char *payload);

#endif