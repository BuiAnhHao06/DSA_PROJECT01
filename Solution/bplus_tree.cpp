#include "bplus_tree.h"

void readNode(int offset, BPlusNode &node)
{
    if (db_file == nullptr)
    {
        std::cerr << "db_file is not open.\n";
        return;
    }

    if (fseek(db_file, offset, SEEK_SET) != 0)
    {
        std::cerr << "Seek before read node failed.\n";
        return;
    }

    size_t result = fread(&node, sizeof(BPlusNode), 1, db_file);

    if (result != 1)
    {
        std::cerr << "Read node failed.\n";
        return;
    }

    disk_read_count++;
}

void writeNode(int offset, BPlusNode &node)
{
    if (db_file == nullptr)
    {
        std::cerr << "db_file is not open.\n";
        return;
    }

    if (fseek(db_file, offset, SEEK_SET) != 0)
    {
        std::cerr << "Seek before write node failed.\n";
        return;
    }

    size_t result = fwrite(&node, sizeof(BPlusNode), 1, db_file);

    if (result != 1)
    {
        std::cerr << "Write node failed.\n";
        return;
    }

    fflush(db_file);

    disk_write_count++;
}

long allocateNode()
{
    long offset = PAGE_SIZE + header.total_nodes * PAGE_SIZE;
    header.total_nodes++;
    writeHeader();
    return offset;
}

void insertRecord(int id, const char *payload)
{
    if (header.root_offset == -1)
    {
        BPlusNode root;

        memset(&root, 0, sizeof(BPlusNode));

        root.is_leaf = true;
        root.num_keys = 1;
        root.leaf.next_leaf_offset = -1;

        root.leaf.records[0].id = id;
        strcpy(root.leaf.records[0].payload, payload);

        int root_offset = allocateNode();

        header.root_offset = root_offset;
        writeHeader();

        writeNode(root_offset, root);

        return;
    }

    BPlusNode root;

    readNode(header.root_offset, root);

    if (root.num_keys >= MAX_LEAF_KEYS)
        return;

    int pos = 0;

    while (pos < root.num_keys && root.leaf.records[pos].id < id)
        pos++;

    for (int i = root.num_keys; i > pos; i--)
    {
        root.leaf.records[i] = root.leaf.records[i - 1];
    }

    root.leaf.records[pos].id = id;
    strcpy(root.leaf.records[pos].payload, payload);

    root.num_keys++;

    writeNode(header.root_offset, root);
}

void readHeader()
{
    if (db_file == nullptr)
        return;

    fseek(db_file, 0, SEEK_SET);

    fread(&header, sizeof(DBHeader), 1, db_file);
}

void writeHeader()
{
    if (db_file == nullptr)
        return;

    fseek(db_file, 0, SEEK_SET);

    fwrite(&header, sizeof(DBHeader), 1, db_file);
    fflush(db_file);
}

void openOrCreateDatabase(const char *filename)
{
    db_file = fopen(filename, "rb+");
    if (db_file != nullptr)
    {
        readHeader();
        return;
    }

    db_file = fopen(filename, "wb+");
    if (db_file == nullptr)
    {
        return;
    }

    header.root_offset = -1;
    header.total_nodes = 0;
    header.free_list_offset = -1;

    writeHeader();
}

int linearSearch(const int keys[], int num_keys, int target)
{
    int i = 0;
    while (i < num_keys && keys[i] < target)
        i++;

    return i;
}
int binarySearch(const int keys[], int num_keys, int target)
{
    int left = 0;
    int right = num_keys;
    while (left < right)
    {
        int mid = (left + right) / 2;
        if (keys[mid] < target)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

bool insertRecursive(int current_offset, int id, const char *payload, int &new_key, int &new_offset)
{
    BPlusNode node;
    readNode(current_offset, node);

    if (node.is_leaf)
    {

        if (node.num_keys < MAX_LEAF_KEYS)
        {
            int pos = 0;

            while (pos < node.num_keys && node.leaf.records[pos].id < id)
                pos++;

            for (int i = node.num_keys; i > pos; i--)
                node.leaf.records[i] = node.leaf.records[i - 1];

            node.leaf.records[pos].id = id;
            strcpy(node.leaf.records[pos].payload, payload);

            node.num_keys++;
            writeNode(current_offset, node);

            return false;
        }


        Record temp[MAX_LEAF_KEYS + 1];

        int pos = 0;

        while (pos < node.num_keys && node.leaf.records[pos].id < id)
            pos++;

        int i, j;

        for (i = 0, j = 0; i < node.num_keys; i++, j++)
        {
            if (j == pos)
                j++;
            temp[j] = node.leaf.records[i];
        }

        temp[pos].id = id;
        strcpy(temp[pos].payload, payload);

        int split = (MAX_LEAF_KEYS + 1) / 2;

        BPlusNode new_leaf;

        memset(&new_leaf, 0, sizeof(BPlusNode));

        new_leaf.is_leaf = true;
        node.num_keys = split;
        new_leaf.num_keys = (MAX_LEAF_KEYS + 1) - split;

        // leaf trái
        for (i = 0; i < split; i++)
            node.leaf.records[i] = temp[i];

        // leaf phải
        for (i = 0; i < new_leaf.num_keys; i++)
            new_leaf.leaf.records[i] = temp[split + i];

        new_offset = allocateNode();

        new_leaf.leaf.next_leaf_offset = node.leaf.next_leaf_offset;

        node.leaf.next_leaf_offset = new_offset;

        writeNode(current_offset, node);
        writeNode(new_offset, new_leaf);

        new_key = new_leaf.leaf.records[0].id;

        return true;
    }

    return false;
}
