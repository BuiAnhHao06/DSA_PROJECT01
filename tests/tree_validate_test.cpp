#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

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

static bool isValidNodeOffset(int offset)
{
    int max_valid_offset = header.total_nodes * PAGE_SIZE;
    return offset >= PAGE_SIZE && offset <= max_valid_offset && offset % PAGE_SIZE == 0;
}

static int findLeftmostLeafOffset()
{
    int current_offset = header.root_offset;
    std::set<int> visited_internal_offsets;

    while (current_offset != -1)
    {
        if (visited_internal_offsets.count(current_offset) > 0)
        {
            std::cerr << "FAIL: cycle detected while descending to leftmost leaf\n";
            return -1;
        }
        visited_internal_offsets.insert(current_offset);

        BPlusNode node;
        std::memset(&node, 0, sizeof(node));
        readNode(current_offset, node);

        if (node.is_leaf)
        {
            return current_offset;
        }

        current_offset = node.internal.children_offsets[0];
    }

    return -1;
}

static bool validateLeafList(int expected_record_count)
{
    bool ok = true;
    int current_offset = findLeftmostLeafOffset();
    ok &= expect(current_offset != -1, "leftmost leaf should be reachable");

    std::set<int> visited_leaf_offsets;
    int total_records = 0;
    int previous_id = 0;

    while (current_offset != -1)
    {
        ok &= expect(isValidNodeOffset(current_offset), "leaf offset should be page-aligned and allocated");
        ok &= expect(visited_leaf_offsets.count(current_offset) == 0,
                     "leaf linked list should not contain a cycle");

        if (!isValidNodeOffset(current_offset) || visited_leaf_offsets.count(current_offset) > 0)
        {
            return false;
        }

        visited_leaf_offsets.insert(current_offset);

        BPlusNode leaf;
        std::memset(&leaf, 0, sizeof(leaf));
        readNode(current_offset, leaf);

        ok &= expect(leaf.is_leaf, "next_leaf_offset should point only to leaf nodes");
        ok &= expect(leaf.num_keys > 0, "leaf should not be empty");
        ok &= expect(leaf.num_keys <= MAX_LEAF_KEYS, "leaf should not exceed MAX_LEAF_KEYS");

        for (int i = 0; i < leaf.num_keys; i++)
        {
            int id = leaf.leaf.records[i].id;
            ok &= expect(id > previous_id, "IDs should increase strictly while traversing leaves");
            previous_id = id;
            total_records++;
        }

        current_offset = leaf.leaf.next_leaf_offset;
    }

    ok &= expect(total_records == expected_record_count,
                 "leaf traversal should count exactly the inserted number of records");
    ok &= expect(previous_id == expected_record_count,
                 "last ID in leaf traversal should equal N");

    std::cout << "DEBUG validated_records=" << total_records
              << " visited_leaves=" << visited_leaf_offsets.size() << "\n";

    return ok;
}

static bool runValidateCase(int record_count, const char *path)
{
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file for N=" << record_count << "\n";
        return false;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "tree-validate-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 7919) % record_count) + 1;
        insertRecord(id, payload);
    }

    std::cout << "DEBUG validating_N=" << record_count
              << " root_offset=" << header.root_offset
              << " total_nodes=" << header.total_nodes << "\n";

    bool ok = true;
    ok &= expect(header.root_offset != -1, "root_offset should be set after insert");
    ok &= expect(isValidNodeOffset(header.root_offset), "root_offset should be valid");
    ok &= validateLeafList(record_count);

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    return ok;
}

int main()
{
    bool ok = true;
    ok &= runValidateCase(1000, "/tmp/dsa_project01_tree_validate_1000.dat");
    ok &= runValidateCase(3000, "/tmp/dsa_project01_tree_validate_3000.dat");
    ok &= runValidateCase(10000, "/tmp/dsa_project01_tree_validate_10000.dat");

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: tree leaf validation test completed successfully.\n";
    return 0;
}
