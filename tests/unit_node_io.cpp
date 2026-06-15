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
    const char *path = "/tmp/dsa_project01_unit_node_io.dat";
    std::remove(path);

    db_file = std::fopen(path, "w+b");
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not open temp database file\n";
        return 1;
    }

    BPlusNode written_node;
    std::memset(&written_node, 0, sizeof(written_node));
    written_node.is_leaf = true;
    written_node.num_keys = 1;
    written_node.leaf.records[0].id = 42;
    std::strncpy(written_node.leaf.records[0].payload, "first-record", PAYLOAD_SIZE - 1);
    written_node.leaf.next_leaf_offset = -1;

    resetIOCounters();
    writeNode(PAGE_SIZE, written_node);

    BPlusNode read_back;
    std::memset(&read_back, 0, sizeof(read_back));
    readNode(PAGE_SIZE, read_back);

    bool ok = true;
    ok &= expect(disk_write_count == 1, "writeNode should increment disk_write_count after successful fwrite");
    ok &= expect(disk_read_count == 1, "readNode should increment disk_read_count after successful fread");
    ok &= expect(read_back.is_leaf == true, "read node should have is_leaf=true");
    ok &= expect(read_back.num_keys == 1, "read node should have num_keys=1");
    ok &= expect(read_back.leaf.records[0].id == 42, "first record id should be preserved");
    ok &= expect(std::strcmp(read_back.leaf.records[0].payload, "first-record") == 0,
                 "first record payload should be preserved");

    std::fclose(db_file);
    db_file = nullptr;

    db_file = std::fopen(path, "rb");
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not reopen temp database file as read-only\n";
        return 1;
    }

    disk_write_count = 7;
    writeNode(PAGE_SIZE, written_node);
    ok &= expect(disk_write_count == 7, "failed fwrite should not increment disk_write_count");

    std::fclose(db_file);
    db_file = nullptr;

    std::remove(path);
    db_file = std::fopen(path, "w+b");
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create empty temp database file\n";
        return 1;
    }

    disk_read_count = 5;
    readNode(PAGE_SIZE, read_back);
    ok &= expect(disk_read_count == 5, "failed fread should not increment disk_read_count");

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: node leaf write/read unit test completed successfully.\n";
    return 0;
}
