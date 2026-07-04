#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <cstring>
#include "bplus_tree_disk.h"

extern FILE *db_file;
extern DBHeader header;

extern int disk_read_count;
extern int disk_write_count;

void resetIOCounters();

void readNode(int offset, BPlusNode &node);

void writeNode(int offset, BPlusNode &node);

long allocateNode();

void insertRecord(int id, const char *payload);

void readHeader();

void writeHeader();

void openOrCreateDatabase(const char *filename);

void buildOrLoadDatabase(int N);

int linearSearch(const int keys[], int num_keys, int target);

int binarySearch(const int keys[], int num_keys, int target);

bool insertRecursive(int current_offset, int id, const char *payload, int &new_key, int &new_offset);

//

int chooseChildLinear(const BPlusNode &node, int target);

int chooseChildBinary(const BPlusNode &node, int target);

int findLeafRecursive(int current_offset, int target, bool use_binary_search);

int findStartLeaf(int start_id, bool use_binary_search);

int rangeRecursive(int current_offset, int start_id, int end_id, bool use_binary_search);

#endif