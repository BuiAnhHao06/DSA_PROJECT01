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

static bool comparePointQueries(int target_id, bool use_binary_search)
{
    resetIOCounters();
    bool bplus_found = pointQueryBPlusStyle(target_id, use_binary_search);
    int bplus_reads = disk_read_count;

    resetIOCounters();
    bool btree_found = pointQueryBTreeStyle(target_id, use_binary_search);
    int btree_reads = disk_read_count;

    bool ok = true;
    ok &= expect(bplus_found == btree_found,
                 "B+ style and B-Tree style point query should return the same result");
    ok &= expect(bplus_reads > 0, "B+ style point query should read at least one page");
    ok &= expect(btree_reads > 0, "B-Tree style point query should read at least one page");

    std::cout << "DEBUG target_id=" << target_id
              << " use_binary_search=" << use_binary_search
              << " bplus_found=" << bplus_found
              << " btree_found=" << btree_found
              << " bplus_reads=" << bplus_reads
              << " btree_reads=" << btree_reads << "\n";

    return ok;
}

int main()
{
    const int record_count = 1000;
    const char *path = "/tmp/dsa_project01_point_query_compare_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "point-query-compare-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 37) % record_count) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;
    for (int i = 0; i < 100; i++)
    {
        int target_id;
        if (i < 50)
        {
            target_id = ((i * 19) % record_count) + 1;
        }
        else
        {
            target_id = record_count + 1 + (i - 50) * 7;
        }

        ok &= comparePointQueries(target_id, false);
        ok &= comparePointQueries(target_id, true);
    }

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: B+ and B-Tree point query comparison test completed successfully.\n";
    return 0;
}
