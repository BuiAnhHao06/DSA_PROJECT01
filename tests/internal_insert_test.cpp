#include <cstdio>
#include <cstring>
#include <iostream>

#include "bplus_tree.h"

FILE *db_file = nullptr;
DBHeader header;
int disk_read_count = 0;
int disk_write_count = 0;

void resetIOCounters()
{
    disk_read_count = 0;
    disk_write_count = 0;
}

static bool expect(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

static void printRootDebug(const BPlusNode &root)
{
    std::cout << "DEBUG root_offset=" << header.root_offset << "\n";
    std::cout << "DEBUG root_num_keys=" << root.num_keys << "\n";

    std::cout << "DEBUG root_keys:";
    for (int i = 0; i < root.num_keys; i++)
    {
        std::cout << " " << root.internal.keys[i];
    }
    std::cout << "\n";

    std::cout << "DEBUG root_children_offsets:";
    for (int i = 0; i <= root.num_keys; i++)
    {
        std::cout << " " << root.internal.children_offsets[i];
    }
    std::cout << "\n";
}

int main()
{
    const char *path = "/tmp/dsa_project01_internal_insert_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "internal-insert-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < 1000; i++)
    {
        int id = ((i * 37) % 1000) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;
    ok &= expect(header.root_offset != -1, "root_offset should be set after inserting records");
    ok &= expect(header.root_offset % PAGE_SIZE == 0, "root_offset should be page-aligned");
    ok &= expect(header.total_nodes > 1, "1000 inserts should allocate internal and leaf nodes");

    BPlusNode root;
    std::memset(&root, 0, sizeof(root));
    readNode(header.root_offset, root);

    printRootDebug(root);

    ok &= expect(!root.is_leaf, "root should be an internal node after inserting 1000 records");
    ok &= expect(root.num_keys > 0, "internal root should contain at least one separator key");
    ok &= expect(root.num_keys <= MAX_INTERNAL_KEYS, "internal root should not exceed MAX_INTERNAL_KEYS");

    for (int i = 1; i < root.num_keys; i++)
    {
        ok &= expect(root.internal.keys[i - 1] < root.internal.keys[i],
                     "root separator keys should be sorted ascending");
    }

    int max_valid_offset = header.total_nodes * PAGE_SIZE;
    for (int i = 0; i <= root.num_keys; i++)
    {
        int child_offset = root.internal.children_offsets[i];
        ok &= expect(child_offset != -1, "root child offset should not be -1");
        ok &= expect(child_offset >= PAGE_SIZE, "root child offset should point after the header page");
        ok &= expect(child_offset <= max_valid_offset, "root child offset should be within allocated pages");
        ok &= expect(child_offset % PAGE_SIZE == 0, "root child offset should be a multiple of PAGE_SIZE");

        BPlusNode child;
        std::memset(&child, 0, sizeof(child));
        readNode(child_offset, child);
        ok &= expect(child.num_keys > 0, "root child should reference a non-empty node");
    }

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: internal insert root child-offset test completed successfully.\n";
    return 0;
}
