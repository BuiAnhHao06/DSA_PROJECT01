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

void insertRecord(FILE *fp, DBHeader &header, int id, const char *payload)
{
    // TODO
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
