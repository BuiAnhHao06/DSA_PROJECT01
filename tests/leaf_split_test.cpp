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

static int findLeftmostLeafOffset()
{
    int current_offset = header.root_offset;

    while (current_offset != -1)
    {
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

static bool runSplitCase(int record_count, const char *path)
{
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file for " << record_count << " records\n";
        return false;
    }

    char payload[PAYLOAD_SIZE];
    std::memset(payload, 'S', PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 37) % record_count) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;
    ok &= expect(header.root_offset != -1, "root_offset should be set after inserts");
    ok &= expect(header.total_nodes > 1, "inserting more than 60 records should split into multiple nodes");

    BPlusNode root;
    std::memset(&root, 0, sizeof(root));
    readNode(header.root_offset, root);
    ok &= expect(!root.is_leaf, "root should become internal after leaf split");

    int current_offset = findLeftmostLeafOffset();
    ok &= expect(current_offset != -1, "leftmost leaf should be reachable from root");

    int expected_id = 1;
    int visited_leaf_count = 0;

    while (current_offset != -1)
    {
        BPlusNode leaf;
        std::memset(&leaf, 0, sizeof(leaf));
        readNode(current_offset, leaf);

        ok &= expect(leaf.is_leaf, "next_leaf_offset chain should visit only leaf nodes");
        ok &= expect(leaf.num_keys > 0, "leaf in linked list should not be empty");
        ok &= expect(leaf.num_keys <= MAX_LEAF_KEYS, "leaf should not exceed MAX_LEAF_KEYS after split");

        visited_leaf_count++;

        for (int i = 0; i < leaf.num_keys; i++)
        {
            ok &= expect(leaf.leaf.records[i].id == expected_id,
                         "leaf linked list should expose records in ascending ID order");
            ok &= expect(std::strcmp(leaf.leaf.records[i].payload, payload) == 0,
                         "payload should be preserved after leaf split");
            expected_id++;
        }

        current_offset = leaf.leaf.next_leaf_offset;
    }

    ok &= expect(visited_leaf_count >= 2, "split case should produce at least two leaves");
    ok &= expect(expected_id == record_count + 1, "leaf linked list should expose every inserted record exactly once");

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    return ok;
}

int main()
{
    bool ok = true;
    ok &= runSplitCase(61, "/tmp/dsa_project01_leaf_split_61.dat");
    ok &= runSplitCase(100, "/tmp/dsa_project01_leaf_split_100.dat");
    ok &= runSplitCase(200, "/tmp/dsa_project01_leaf_split_200.dat");

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: leaf split linked-list test completed successfully.\n";
    return 0;
}
