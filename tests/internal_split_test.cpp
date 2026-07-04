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

static bool isValidNodeOffset(int offset)
{
    int max_valid_offset = header.total_nodes * PAGE_SIZE;
    return offset >= PAGE_SIZE && offset <= max_valid_offset && offset % PAGE_SIZE == 0;
}

static int debugTreeHeight(int offset)
{
    if (offset == -1)
    {
        std::cout << "DEBUG tree_height=0\n";
        return 0;
    }

    int height = 0;
    int current_offset = offset;

    while (current_offset != -1)
    {
        BPlusNode node;
        std::memset(&node, 0, sizeof(node));
        readNode(current_offset, node);

        height++;
        std::cout << "DEBUG level=" << height
                  << " offset=" << current_offset
                  << " is_leaf=" << node.is_leaf
                  << " num_keys=" << node.num_keys << "\n";

        if (node.is_leaf)
        {
            break;
        }

        current_offset = node.internal.children_offsets[0];
    }

    std::cout << "DEBUG tree_height=" << height << "\n";
    return height;
}

static bool validateInternalShape(int offset, int depth, int &leaf_depth)
{
    BPlusNode node;
    std::memset(&node, 0, sizeof(node));
    readNode(offset, node);

    bool ok = true;
    ok &= expect(node.num_keys > 0, "visited node should not be empty");

    if (node.is_leaf)
    {
        if (leaf_depth == -1)
        {
            leaf_depth = depth;
        }
        ok &= expect(depth == leaf_depth, "all leaves should appear at the same tree depth");
        ok &= expect(node.num_keys <= MAX_LEAF_KEYS, "leaf should not exceed MAX_LEAF_KEYS");
        return ok;
    }

    ok &= expect(node.num_keys <= MAX_INTERNAL_KEYS, "internal node should not exceed MAX_INTERNAL_KEYS");
    for (int i = 1; i < node.num_keys; i++)
    {
        ok &= expect(node.internal.keys[i - 1] < node.internal.keys[i],
                     "internal keys should be sorted ascending");
    }

    int child_count = node.num_keys + 1;
    ok &= expect(child_count >= 2, "internal node should have num_keys + 1 children");

    for (int i = 0; i < child_count; i++)
    {
        int child_offset = node.internal.children_offsets[i];
        ok &= expect(child_offset != -1, "internal child offset should not be -1");
        ok &= expect(isValidNodeOffset(child_offset),
                     "internal child offset should be page-aligned and inside allocated pages");

        if (isValidNodeOffset(child_offset))
        {
            ok &= validateInternalShape(child_offset, depth + 1, leaf_depth);
        }
    }

    return ok;
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

static bool validateLeafLinkedList(int record_count)
{
    int current_offset = findLeftmostLeafOffset();
    bool ok = true;
    ok &= expect(current_offset != -1, "leftmost leaf should be reachable");

    int expected_id = 1;
    int visited_leaf_count = 0;

    while (current_offset != -1)
    {
        ok &= expect(isValidNodeOffset(current_offset), "leaf offset should be valid");

        BPlusNode leaf;
        std::memset(&leaf, 0, sizeof(leaf));
        readNode(current_offset, leaf);

        ok &= expect(leaf.is_leaf, "next_leaf_offset chain should contain only leaves");
        ok &= expect(leaf.num_keys > 0, "linked leaf should not be empty");
        ok &= expect(leaf.num_keys <= MAX_LEAF_KEYS, "linked leaf should not exceed MAX_LEAF_KEYS");

        visited_leaf_count++;

        for (int i = 0; i < leaf.num_keys; i++)
        {
            ok &= expect(leaf.leaf.records[i].id == expected_id,
                         "leaf linked list should return records in ascending ID order");
            expected_id++;
        }

        current_offset = leaf.leaf.next_leaf_offset;
    }

    ok &= expect(visited_leaf_count > MAX_INTERNAL_KEYS,
                 "large insert should create more leaves than one internal node can reference");
    ok &= expect(expected_id == record_count + 1,
                 "leaf linked list should expose every inserted record exactly once");

    return ok;
}

int main()
{
    const int record_count = 40000;
    const char *path = "/tmp/dsa_project01_internal_split_test.dat";
    std::remove(path);

    openOrCreateDatabase(path);
    if (db_file == nullptr)
    {
        std::cerr << "FAIL: could not create database file\n";
        return 1;
    }

    char payload[PAYLOAD_SIZE];
    std::strncpy(payload, "internal-split-test", PAYLOAD_SIZE - 1);
    payload[PAYLOAD_SIZE - 1] = '\0';

    for (int i = 0; i < record_count; i++)
    {
        int id = ((i * 7919) % record_count) + 1;
        insertRecord(id, payload);
    }

    bool ok = true;
    ok &= expect(header.root_offset != -1, "root_offset should be set after large insert");
    ok &= expect(isValidNodeOffset(header.root_offset), "root_offset should be a valid page offset");

    BPlusNode root;
    std::memset(&root, 0, sizeof(root));
    readNode(header.root_offset, root);

    std::cout << "DEBUG root_offset=" << header.root_offset << "\n";
    std::cout << "DEBUG root_num_keys=" << root.num_keys << "\n";

    int height = debugTreeHeight(header.root_offset);
    ok &= expect(!root.is_leaf, "root should be internal after large insert");
    ok &= expect(height >= 3, "large insert should split the old internal root and create height >= 3");
    ok &= expect(root.num_keys + 1 >= 2, "root child count should equal root keys + 1");

    int leaf_depth = -1;
    ok &= validateInternalShape(header.root_offset, 1, leaf_depth);
    ok &= validateLeafLinkedList(record_count);

    std::fclose(db_file);
    db_file = nullptr;
    std::remove(path);

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS: internal split tree-height and leaf-chain test completed successfully.\n";
    return 0;
}
