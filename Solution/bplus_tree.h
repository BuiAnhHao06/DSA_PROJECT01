#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "bplus_tree_disk.h"

extern FILE* db_file;
extern DBHeader header;

extern int disk_read_count;
extern int disk_write_count;

void resetIOCounters();

void readNode(int offset, BPlusNode &node);

void writeNode(int offset, BPlusNode &node);

long allocateNode(FILE *fp, DBHeader &header);

void insertRecord(FILE *fp, DBHeader &header, int id, const char *payload);

#endif