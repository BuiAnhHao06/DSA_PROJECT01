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

static bool runPointQuery(int target_id, bool use_binary_search, bool expected, const char *message)
{
    resetIOCounters();
    bool actual = pointQueryBPlusStyle(target_id, use_binary_search);

    bool ok = true;
    ok &= expect(actual == expected, message);
    ok &= expect(disk_read_count > 0, "point query should read at least one page");

    std::cout << "DEBUG target_id=" << target_id
              << " use_binary_search=" << use_binary_search
              << " found=" << actual
              << " disk_reads=" << disk_read_count << "\n";

    return ok;
}

int main()
{
    const char *path = "/tmp/dsa_project01_point_query_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "point-query-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < 1000; i++)
    {
        int id = ((i * 37) % 1000) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;
    ok &= runPointQuery(1, false, true, "linear point query should find first inserted-range ID");
    ok &= runPointQuery(500, false, true, "linear point query should find middle inserted-range ID");
    ok &= runPointQuery(1000, false, true, "linear point query should find last inserted-range ID");
    ok &= runPointQuery(0, false, false, "linear point query should not find ID below inserted range");
    ok &= runPointQuery(1001, false, false, "linear point query should not find ID above inserted range");

    ok &= runPointQuery(1, true, true, "binary point query should find first inserted-range ID");
    ok &= runPointQuery(500, true, true, "binary point query should find middle inserted-range ID");
    ok &= runPointQuery(1000, true, true, "binary point query should find last inserted-range ID");
    ok &= runPointQuery(0, true, false, "binary point query should not find ID below inserted range");
    ok &= runPointQuery(1001, true, false, "binary point query should not find ID above inserted range");

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: B+ Tree point query test completed successfully.\n";
    return 0;
}
