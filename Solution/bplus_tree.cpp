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

long allocateNode(FILE *fp, DBHeader &header)
{
    // TODO
    return -1;
}

void insertRecord(FILE *fp, DBHeader &header, int id, const char *payload)
{
    // TODO
}
