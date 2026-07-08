#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>
#include <vector>
#include "bplus_tree_disk.h"
#include "bplus_tree.h"

using namespace std;
using namespace std::chrono;

FILE *db_file = nullptr;
DBHeader header;

struct AppConfig
{
    string dataset_file = "dataset_large.csv";
    string index_prefix = "index_";
    int query_count = 10;
    int range_threshold = 2000;
    int range_small = 50;
    int range_large = 100;
    vector<int> data_sizes = {100, 500, 1000};
};

static string trim(const string &value)
{
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == string::npos)
    {
        return "";
    }
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

static vector<int> parseIntList(const string &value)
{
    vector<int> result;
    stringstream ss(value);
    string item;

    while (getline(ss, item, ','))
    {
        item = trim(item);
        if (!item.empty())
        {
            result.push_back(stoi(item));
        }
    }

    return result;
}

static AppConfig loadConfig(const string &path)
{
    AppConfig config;
    ifstream file(path);
    if (!file.is_open())
    {
        cout << "Config file '" << path << "' not found. Using defaults.\n";
        return config;
    }

    string line;
    while (getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t separator = line.find('=');
        if (separator == string::npos)
        {
            continue;
        }

        string key = trim(line.substr(0, separator));
        string value = trim(line.substr(separator + 1));

        if (key == "dataset_file")
        {
            config.dataset_file = value;
        }
        else if (key == "index_prefix")
        {
            config.index_prefix = value;
        }
        else if (key == "query_count")
        {
            config.query_count = stoi(value);
        }
        else if (key == "range_threshold")
        {
            config.range_threshold = stoi(value);
        }
        else if (key == "range_small")
        {
            config.range_small = stoi(value);
        }
        else if (key == "range_large")
        {
            config.range_large = stoi(value);
        }
        else if (key == "data_sizes")
        {
            vector<int> parsed_sizes = parseIntList(value);
            if (!parsed_sizes.empty())
            {
                config.data_sizes = parsed_sizes;
            }
        }
    }

    return config;
}

// TODO for students: Implement Disk I/O counters and helper functions:
// int disk_read_count = 0;
// int disk_write_count = 0;
// void resetIOCounters();

int disk_read_count = 0;
int disk_write_count = 0;
void resetIOCounters()
{
    disk_read_count = 0;
    disk_write_count = 0;
}

// TODO for students: Implement page read/write, search, insert, and query functions...
// void readNode(int offset, BPlusNode& node);
// void writeNode(int offset, BPlusNode& node);
// int linearSearch(const int keys[], int num_keys, int target);
// int binarySearch(const int keys[], int num_keys, int target);
// bool insertRecursive(int current_offset, int id, const char* payload, int& new_key, int& new_offset);
// void insertRecord(int id, const char* payload);
// bool pointQueryBPlusStyle(int target_id, bool use_binary_search);
// bool pointQueryBTreeStyle(int target_id, bool use_binary_search);
// int rangeQueryBPlusStyle(int start_id, int end_id, bool use_binary_search);
// int rangeQueryBTreeStyle(int start_id, int end_id, bool use_binary_search);

void runBenchmark(int N, const vector<int> &query_ids, const AppConfig &config)
{
    cout << "\n========== BENCHMARK WITH N = " << N << " ==========\n";

    int range_size = (N < config.range_threshold) ? config.range_small : config.range_large;

    // TODO for students: Run config.query_count queries (using query_ids) for each of the 8 scenarios.
    // Calculate and print the average Execution Time (ns) and Disk Reads.
    // Hint: start_id for Range Query is query_ids[j], and end_id is query_ids[j] + range_size.
    cout << "Configured query count: " << query_ids.size() << "\n";
    cout << "Configured range size: " << range_size << "\n";
}

int main()
{
    AppConfig config = loadConfig("config.txt");

    if (config.query_count <= 0)
    {
        cerr << "Error: query_count must be positive.\n";
        return 1;
    }
    if (config.data_sizes.empty())
    {
        cerr << "Error: data_sizes must contain at least one value.\n";
        return 1;
    }

    // Preload query IDs from the configured dataset to serve as benchmark target IDs
    vector<int> query_ids;
    query_ids.reserve(config.query_count);

    ifstream csv_setup_file(config.dataset_file);
    if (!csv_setup_file.is_open())
    {
        cerr << "Error: Could not find " << config.dataset_file << ". Run data_generator.cpp first!\n";
        return 1;
    }
    string setup_line;
    getline(csv_setup_file, setup_line); // Skip header row
    while ((int)query_ids.size() < config.query_count && getline(csv_setup_file, setup_line))
    {
        stringstream ss(setup_line);
        string id_str;
        getline(ss, id_str, ',');
        query_ids.push_back(stoi(id_str));
    }
    csv_setup_file.close();

    if ((int)query_ids.size() < config.query_count)
    {
        cerr << "Error: " << config.dataset_file << " does not contain enough records ("
             << config.query_count << " required) for benchmarking!\n";
        return 1;
    }

    cout << "Dataset file: " << config.dataset_file << "\n";
    cout << "Query count: " << config.query_count << "\n";

    for (size_t i = 0; i < config.data_sizes.size(); ++i)
    {
        int N = config.data_sizes[i];
        string filename = config.index_prefix + to_string(N) + ".dat";

        // TODO for students: Open or initialize the index_N.dat binary DB file at offset 0
        // ...
        cout << "\n========================================\n";
        cout << "Preparing database with N = " << N << "\n";
        cout << "========================================\n";

        buildOrLoadDatabase(filename.c_str(), config.dataset_file.c_str(), N);

        cout << "Database file : " << filename << "\n";
        // DATA SIZE WARNING: dataset_large.csv contains 10,000,000 rows (~700MB). Reading and writing
        // this file can take 5 - 15 minutes. It is highly recommended to debug with N <= 1,000,000 first.
        // If the DB is new, insert the first N records from dataset_large.csv
        // TODO for students: Implement the CSV reading and insertion logic for the first N records
        /*
        if (header.root_offset == -1) {
            cout << "Importing " << N << " records...\n";
            ifstream csv_file(config.dataset_file);
            if (!csv_file.is_open()) {
                cerr << "Error: Could not find " << config.dataset_file << "!\n";
                // fclose(db_file);
                return 1;
            }

            string line;
            getline(csv_file, line); // Skip header row

            int count = 0;
            while (count < N && getline(csv_file, line)) {
                // Parse ID and Payload from the CSV line
                // Call insertRecord(...)
                count++;
            }
            csv_file.close();
        }
        */

        // Run benchmark on this database file
        runBenchmark(N, query_ids, config);

        // TODO for students: Close the file
        // fclose(db_file);
        // db_file = nullptr;
        if (db_file != nullptr)
        {
            fclose(db_file);
            db_file = nullptr;
        }
    }
    return 0;
}
