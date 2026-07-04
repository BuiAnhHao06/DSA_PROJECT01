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

static bool runRangeQuery(int start_id, int end_id, bool use_binary_search, int expected, const char *message)
{
    resetIOCounters();
    int actual = rangeQueryBPlusStyle(start_id, end_id, use_binary_search);

    bool ok = true;
    ok &= expect(actual == expected, message);

    if (start_id <= end_id)
    {
        ok &= expect(disk_read_count > 0, "range query should read at least one page");
    }
    else
    {
        ok &= expect(disk_read_count == 0, "invalid empty range should return before reading pages");
    }

    std::cout << "DEBUG range=[" << start_id << "," << end_id << "]"
              << " use_binary_search=" << use_binary_search
              << " count=" << actual
              << " disk_reads=" << disk_read_count << "\n";

    return ok;
}

int main()
{
    const int record_count = 1000;
    const char *path = "/tmp/dsa_project01_range_query_bplus_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "range-query-bplus-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 37) % record_count) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;

    ok &= runRangeQuery(10, 15, false, 6, "linear small range should count records within one leaf");
    ok &= runRangeQuery(10, 15, true, 6, "binary small range should count records within one leaf");

    ok &= runRangeQuery(25, 175, false, 151, "linear multi-leaf range should count all matching records");
    ok &= runRangeQuery(25, 175, true, 151, "binary multi-leaf range should count all matching records");

    ok &= runRangeQuery(995, 1005, false, 6, "linear range crossing upper dataset boundary should stop at last record");
    ok &= runRangeQuery(995, 1005, true, 6, "binary range crossing upper dataset boundary should stop at last record");

    ok &= runRangeQuery(1001, 1010, false, 0, "linear empty range above dataset should return 0");
    ok &= runRangeQuery(1001, 1010, true, 0, "binary empty range above dataset should return 0");

    ok &= runRangeQuery(500, 499, false, 0, "linear invalid empty range should return 0");
    ok &= runRangeQuery(500, 499, true, 0, "binary invalid empty range should return 0");

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: B+ Tree range query test completed successfully.\n";
    return 0;
}
