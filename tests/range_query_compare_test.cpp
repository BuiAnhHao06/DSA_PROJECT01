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

static bool compareRangeQueries(int start_id, int end_id, bool use_binary_search)
{
    resetIOCounters();
    int bplus_count = rangeQueryBPlusStyle(start_id, end_id, use_binary_search);
    int bplus_reads = disk_read_count;

    resetIOCounters();
    int btree_count = rangeQueryBTreeStyle(start_id, end_id, use_binary_search);
    int btree_reads = disk_read_count;

    bool ok = true;
    ok &= expect(bplus_count == btree_count,
                 "B+ style and B-Tree style range query should return the same count");

    std::cout << "DEBUG range=[" << start_id << "," << end_id << "]"
              << " use_binary_search=" << use_binary_search
              << " bplus_count=" << bplus_count
              << " btree_count=" << btree_count
              << " bplus_reads=" << bplus_reads
              << " btree_reads=" << btree_reads << "\n";

    return ok;
}

static bool runCompareCase(int record_count, const char *path)
{
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file for N=" << record_count << "\n";
        return false;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "range-query-compare-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 7919) % record_count) + 1;
        insertRecord(id, payload);
    }

    std::cout << "DEBUG compare_N=" << record_count
              << " root_offset=" << header.root_offset
              << " total_nodes=" << header.total_nodes << "\n";

    const int ranges[][2] = {
        {1, 1},
        {10, 15},
        {25, 175},
        {record_count / 2 - 25, record_count / 2 + 25},
        {record_count - 75, record_count + 75},
        {record_count + 1, record_count + 100},
        {500, 499},
    };

    bool ok = true;
    for (int i = 0; i < 7; i++)
    {
        ok &= compareRangeQueries(ranges[i][0], ranges[i][1], false);
        ok &= compareRangeQueries(ranges[i][0], ranges[i][1], true);
    }

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    return ok;
}

int main()
{
    bool ok = true;
    ok &= runCompareCase(1000, "/tmp/dsa_project01_range_compare_1000.dat");
    ok &= runCompareCase(3000, "/tmp/dsa_project01_range_compare_3000.dat");
    ok &= runCompareCase(10000, "/tmp/dsa_project01_range_compare_10000.dat");

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: B+ and B-Tree range query comparison test completed successfully.\n";
    return 0;
}
