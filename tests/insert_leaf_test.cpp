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

int main()
{
    const char *path = "/tmp/dsa_project01_insert_leaf_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    const int ids[] = {50, 10, 90, 30, 70, 20, 100, 40, 80, 60};
    const int expected_ids[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    char payload[PAYLOAD_SIZE];
    std::memset(payload, 'X', PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < 10; i++)
    {
        insertRecord(ids[i], payload);
    }

    bool ok = true;
    ok &= expect(header.root_offset == PAGE_SIZE, "root should be the first allocated node");
    ok &= expect(header.total_nodes == 1, "inserting 10 records should not split the leaf");

    BPlusNode root;
    std::memset(&root, 0, sizeof(root));
    readNode(header.root_offset, root);

    ok &= expect(root.is_leaf, "root should still be a leaf before split");
    ok &= expect(root.num_keys == 10, "leaf num_keys should equal inserted record count");
    ok &= expect(root.leaf.next_leaf_offset == -1, "leaf should not point to another leaf before split");

    for (int i = 0; i < 10; i++)
    {
        ok &= expect(root.leaf.records[i].id == expected_ids[i], "leaf record IDs should be sorted ascending");
        ok &= expect(std::strlen(root.leaf.records[i].payload) == PAYLOAD_SIZE - 1,
                     "payload should preserve maximum 59-character string");
        ok &= expect(root.leaf.records[i].payload[PAYLOAD_SIZE - 1] == '\0',
                     "payload should have a null terminator at the last byte");
    }

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: insert leaf without split test completed successfully.\n";
    return 0;
}
