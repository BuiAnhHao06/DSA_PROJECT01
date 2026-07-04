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

static bool expectEqual(int actual, int expected, const char *message)
{
    if (actual != expected)
    {
        std::cerr << "FAIL: " << message << " (expected " << expected
                  << ", got " << actual << ")\n";
        return false;
    }
    return true;
}

static bool expectBothSearches(const int keys[], int num_keys, int target, int expected, const char *message)
{
    bool ok = true;
    ok &= expectEqual(linearSearch(keys, num_keys, target), expected, message);
    ok &= expectEqual(binarySearch(keys, num_keys, target), expected, message);
    return ok;
}

int main()
{
    bool ok = true;

    int empty_keys[1] = {0};
    ok &= expectBothSearches(empty_keys, 0, 10, 0, "empty array should return insertion index 0");

    int one_key[] = {20};
    ok &= expectBothSearches(one_key, 1, 10, 0, "single key: target smaller than key should return 0");
    ok &= expectBothSearches(one_key, 1, 20, 0, "single key: target equal to key should return 0");
    ok &= expectBothSearches(one_key, 1, 30, 1, "single key: target larger than key should return 1");

    int three_keys[] = {10, 20, 30};
    ok &= expectBothSearches(three_keys, 3, 5, 0, "multiple keys: target smaller than all keys should return 0");
    ok &= expectBothSearches(three_keys, 3, 10, 0, "multiple keys: target equal to first key should return 0");
    ok &= expectBothSearches(three_keys, 3, 25, 2, "multiple keys: target 25 should return 2");
    ok &= expectBothSearches(three_keys, 3, 30, 2, "multiple keys: target 30 should return 2");
    ok &= expectBothSearches(three_keys, 3, 40, 3, "multiple keys: target 40 should return 3");

    int leaf_sized_keys[MAX_LEAF_KEYS];
    for (int i = 0; i < MAX_LEAF_KEYS; i++)
    {
        leaf_sized_keys[i] = (i + 1) * 10;
    }
    ok &= expectBothSearches(leaf_sized_keys, MAX_LEAF_KEYS, 5, 0,
                             "60-key array: target smaller than all keys should return 0");
    ok &= expectBothSearches(leaf_sized_keys, MAX_LEAF_KEYS, 300, 29,
                             "60-key array: target equal to middle key should return its index");
    ok &= expectBothSearches(leaf_sized_keys, MAX_LEAF_KEYS, 305, 30,
                             "60-key array: target between middle keys should return insertion index");
    ok &= expectBothSearches(leaf_sized_keys, MAX_LEAF_KEYS, 700, MAX_LEAF_KEYS,
                             "60-key array: target larger than all keys should return num_keys");

    int internal_sized_keys[MAX_INTERNAL_KEYS];
    for (int i = 0; i < MAX_INTERNAL_KEYS; i++)
    {
        internal_sized_keys[i] = (i + 1) * 2;
    }
    ok &= expectBothSearches(internal_sized_keys, MAX_INTERNAL_KEYS, 1, 0,
                             "510-key array: target smaller than all keys should return 0");
    ok &= expectBothSearches(internal_sized_keys, MAX_INTERNAL_KEYS, 510, 254,
                             "510-key array: target equal to middle key should return its index");
    ok &= expectBothSearches(internal_sized_keys, MAX_INTERNAL_KEYS, 511, 255,
                             "510-key array: target between keys should return insertion index");
    ok &= expectBothSearches(internal_sized_keys, MAX_INTERNAL_KEYS, 2000, MAX_INTERNAL_KEYS,
                             "510-key array: target larger than all keys should return num_keys");

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: search function unit test completed successfully.\n";
    return 0;
}
