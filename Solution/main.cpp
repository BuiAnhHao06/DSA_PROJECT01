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

void runBenchmark(int N, const vector<int> &query_ids, const AppConfig &config, ofstream &results_csv)
{
    cout << "\n========== BENCHMARK WITH N = " << N << " ==========\n";

    int range_size = (N < config.range_threshold) ? config.range_small : config.range_large;

    cout << "Configured query count: " << query_ids.size() << "\n";
    cout << "Configured range size: " << range_size << "\n";

    struct Scenario
    {
        const char *name;
        const char *query_type;
        const char *style;
        const char *search_type;
        bool is_range_query;
        bool use_bplus_style;
        bool use_binary_search;
    };

    const Scenario scenarios[] = {
        {"Point B-Tree Linear", "point", "BTree", "linear", false, false, false},
        {"Point B-Tree Binary", "point", "BTree", "binary", false, false, true},
        {"Point B+ Tree Linear", "point", "BPlus", "linear", false, true, false},
        {"Point B+ Tree Binary", "point", "BPlus", "binary", false, true, true},
        {"Range B-Tree Linear", "range", "BTree", "linear", true, false, false},
        {"Range B-Tree Binary", "range", "BTree", "binary", true, false, true},
        {"Range B+ Tree Linear", "range", "BPlus", "linear", true, true, false},
        {"Range B+ Tree Binary", "range", "BPlus", "binary", true, true, true},
    };

    for (const Scenario &scenario : scenarios)
    {
        long long total_time_ns = 0;
        long long total_disk_reads = 0;

        for (size_t j = 0; j < query_ids.size(); j++)
        {
            int target_id = query_ids[j];
            resetIOCounters();

            auto start = high_resolution_clock::now();
            if (scenario.is_range_query)
            {
                int end_id = target_id + range_size - 1;
                if (scenario.use_bplus_style)
                {
                    rangeQueryBPlusStyle(target_id, end_id, scenario.use_binary_search);
                }
                else
                {
                    rangeQueryBTreeStyle(target_id, end_id, scenario.use_binary_search);
                }
            }
            else
            {
                if (scenario.use_bplus_style)
                {
                    pointQueryBPlusStyle(target_id, scenario.use_binary_search);
                }
                else
                {
                    pointQueryBTreeStyle(target_id, scenario.use_binary_search);
                }
            }
            auto end = high_resolution_clock::now();

            total_time_ns += duration_cast<nanoseconds>(end - start).count();
            total_disk_reads += disk_read_count;
        }

        double mean_time_ns = static_cast<double>(total_time_ns) / query_ids.size();
        double mean_disk_reads = static_cast<double>(total_disk_reads) / query_ids.size();

        cout << "RESULT | N=" << N
             << " | Scenario=" << scenario.name
             << " | MeanTimeNs=" << mean_time_ns
             << " | MeanDiskReads=" << mean_disk_reads << "\n";

        results_csv << N << ","
                    << scenario.query_type << ","
                    << scenario.style << ","
                    << scenario.search_type << ","
                    << range_size << ","
                    << query_ids.size() << ","
                    << mean_time_ns << ","
                    << mean_disk_reads << "\n";
    }
}

int main() {
    AppConfig config = loadConfig("config.txt");

    if (config.query_count <= 0) {
        cerr << "Error: query_count must be positive.\n";
        return 1;
    }
    if (config.data_sizes.empty()) {
        cerr << "Error: data_sizes must contain at least one value.\n";
        return 1;
    }

    // Preload query IDs from the configured dataset to serve as benchmark target IDs
    vector<int> query_ids;
    query_ids.reserve(config.query_count);

    ifstream csv_setup_file(config.dataset_file);
    if (!csv_setup_file.is_open()) {
        cerr << "Error: Could not find " << config.dataset_file
             << ". Compile and run Solution/data_generator.cpp first!\n";
        return 1;
    }
    string setup_line;
    getline(csv_setup_file, setup_line); // Skip header row
    while ((int)query_ids.size() < config.query_count && getline(csv_setup_file, setup_line)) {
        stringstream ss(setup_line);
        string id_str;
        getline(ss, id_str, ',');
        query_ids.push_back(stoi(id_str));
    }
    csv_setup_file.close();

    if ((int)query_ids.size() < config.query_count) {
        cerr << "Error: " << config.dataset_file << " does not contain enough records ("
             << config.query_count << " required) for benchmarking!\n";
        return 1;
    }

    cout << "Dataset file: " << config.dataset_file << "\n";
    cout << "Query count: " << config.query_count << "\n";

    ofstream results_csv("benchmark_results.csv");
    if (!results_csv.is_open()) {
        cerr << "Error: Could not create benchmark_results.csv!\n";
        return 1;
    }
    results_csv << "N,query_type,style,search_type,range_size,query_count,mean_time_ns,mean_disk_reads\n";

    for (size_t i = 0; i < config.data_sizes.size(); ++i) {
        int N = config.data_sizes[i];
        string filename = config.index_prefix + to_string(N) + ".dat";

        openOrCreateDatabase(filename.c_str());
        if (db_file == nullptr) {
            cerr << "Error: Could not open or create " << filename << "!\n";
            return 1;
        }

        // DATA SIZE WARNING: dataset_large.csv contains 10,000,000 rows (~700MB). Reading and writing
        // this file can take 5 - 15 minutes. It is highly recommended to debug with N <= 1,000,000 first.
        if (header.root_offset == -1) {
            cout << "Importing " << N << " records...\n";
            ifstream csv_file(config.dataset_file);
            if (!csv_file.is_open()) {
                cerr << "Error: Could not find " << config.dataset_file << "!\n";
                fclose(db_file);
                db_file = nullptr;
                return 1;
            }

            string line;
            getline(csv_file, line); // Skip header row

            int count = 0;
            while (count < N && getline(csv_file, line)) {
                stringstream ss(line);
                string id_str;
                string payload_str;

                getline(ss, id_str, ',');
                getline(ss, payload_str);

                char payload[PAYLOAD_SIZE];
                strncpy(payload, payload_str.c_str(), PAYLOAD_SIZE - 1);
                payload[PAYLOAD_SIZE - 1] = '\0';

                insertRecord(stoi(id_str), payload);
                count++;
            }
            csv_file.close();

            if (count < N) {
                cerr << "Error: " << config.dataset_file << " contains only " << count
                     << " data records, but N=" << N << " was requested.\n";
                fclose(db_file);
                db_file = nullptr;
                return 1;
            }
        }

        cout << "Prepared benchmark file name: " << filename << "\n";
        runBenchmark(N, query_ids, config, results_csv);

        fclose(db_file);
        db_file = nullptr;
    }
    results_csv.close();
    cout << "Benchmark CSV: benchmark_results.csv\n";
    return 0;
}
