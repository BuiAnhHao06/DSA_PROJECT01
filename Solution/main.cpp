#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>
#include "bplus_tree_disk.h"

using namespace std;
using namespace std::chrono;

FILE* db_file = nullptr;
DBHeader header;

// TODO for students: Implement Disk I/O counters and helper functions:
// int disk_read_count = 0;
// int disk_write_count = 0;
// void resetIOCounters();

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

void runBenchmark(int N, const int query_ids[]) {
    cout << "\n========== BENCHMARK WITH N = " << N << " ==========\n";
    
    int range_size = (N < 2000) ? 500 : 1000;

    // TODO for students: Run 100 queries (using query_ids) for each of the 8 scenarios.
    // Calculate and print the average Execution Time (ns) and Disk Reads.
    // Hint: start_id for Range Query is query_ids[j], and end_id is query_ids[j] + range_size.
}

int main() {
    // Preload the first 100 IDs from dataset_large.csv to serve as benchmark target IDs
    int query_ids[100];
    ifstream csv_setup_file("dataset_large.csv");
    if (!csv_setup_file.is_open()) {
        cerr << "Error: Could not find dataset_large.csv. Run data_generator.cpp first!\n";
        return 1;
    }
    string setup_line;
    getline(csv_setup_file, setup_line); // Skip header row
    int query_count = 0;
    while (query_count < 100 && getline(csv_setup_file, setup_line)) {
        stringstream ss(setup_line);
        string id_str;
        getline(ss, id_str, ',');
        query_ids[query_count++] = stoi(id_str);
    }
    csv_setup_file.close();

    if (query_count < 100) {
        cerr << "Error: dataset_large.csv does not contain enough records (100 required) for benchmarking!\n";
        return 1;
    }

    const int DATA_SIZES[] = {1000, 3000, 10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    const int NUM_SIZES = sizeof(DATA_SIZES) / sizeof(DATA_SIZES[0]);

    for (int i = 0; i < NUM_SIZES; ++i) {
        int N = DATA_SIZES[i];
        string filename = "index_" + to_string(N) + ".dat";
        
        // TODO for students: Open or initialize the index_N.dat binary DB file at offset 0
        // ...

        // DATA SIZE WARNING: dataset_large.csv contains 10,000,000 rows (~700MB). Reading and writing
        // this file can take 5 - 15 minutes. It is highly recommended to debug with N <= 1,000,000 first.
        // If the DB is new, insert the first N records from dataset_large.csv
        // TODO for students: Implement the CSV reading and insertion logic for the first N records
        /*
        if (header.root_offset == -1) {
            cout << "Importing " << N << " records...\n";
            ifstream csv_file("dataset_large.csv");
            if (!csv_file.is_open()) {
                cerr << "Error: Could not find dataset_large.csv!\n";
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
        // runBenchmark(N, query_ids);

        // TODO for students: Close the file
        // fclose(db_file);
        // db_file = nullptr;
    }
    return 0;
}
