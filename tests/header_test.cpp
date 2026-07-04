#include <cstdio>
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
    const char *path = "/tmp/dsa_project01_header_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    bool ok = true;
    ok &= expect(header.root_offset == -1, "new database should start with root_offset=-1");
    ok &= expect(header.total_nodes == 0, "new database should start with total_nodes=0");
    ok &= expect(header.free_list_offset == -1, "new database should start with free_list_offset=-1");

    long first_offset = allocateNode();
    long second_offset = allocateNode();
    long third_offset = allocateNode();

    ok &= expect(first_offset == PAGE_SIZE, "first allocated node should start after header page");
    ok &= expect(second_offset == PAGE_SIZE * 2, "second allocated node should be page 2");
    ok &= expect(third_offset == PAGE_SIZE * 3, "third allocated node should be page 3");
    ok &= expect(header.total_nodes == 3, "total_nodes should be 3 after allocating three nodes");

    header.root_offset = static_cast<int>(second_offset);
    writeHeader();

    std::fclose(db_file);
    db_file = nullptr;

    header.root_offset = 12345;
    header.total_nodes = 12345;
    header.free_list_offset = 12345;

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not reopen database file\n";
        return 1;
    }

    ok &= expect(header.root_offset == second_offset, "reopened database should preserve root_offset");
    ok &= expect(header.total_nodes == 3, "reopened database should preserve total_nodes");
    ok &= expect(header.free_list_offset == -1, "reopened database should preserve free_list_offset");

    long fourth_offset = allocateNode();
    ok &= expect(fourth_offset == PAGE_SIZE * 4, "next allocation after reopen should continue at page 4");
    ok &= expect(header.total_nodes == 4, "total_nodes should continue from persisted value after reopen");

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: header persistence test completed successfully.\n";
    return 0;
}
