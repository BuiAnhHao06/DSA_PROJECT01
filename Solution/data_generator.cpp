#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct GeneratorConfig {
    string dataset_file = "dataset_large.csv";
    int generated_record_count = 1000;
};

static string trim(const string &value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        return "";
    }
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

static GeneratorConfig loadGeneratorConfig(const string &path) {
    GeneratorConfig config;
    ifstream file(path);
    if (!file.is_open()) {
        cout << "Config file '" << path << "' not found. Using defaults.\n";
        return config;
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t separator = line.find('=');
        if (separator == string::npos) {
            continue;
        }

        string key = trim(line.substr(0, separator));
        string value = trim(line.substr(separator + 1));

        if (key == "dataset_file") {
            config.dataset_file = value;
        } else if (key == "generated_record_count") {
            config.generated_record_count = stoi(value);
        }
    }

    return config;
}

int main() {
    GeneratorConfig config = loadGeneratorConfig("config.txt");
    const int num_records = config.generated_record_count;

    if (num_records <= 0) {
        cerr << "Error: generated_record_count must be positive.\n";
        return 1;
    }

    cout << "Generating " << num_records << " random records. Please wait..." << endl;

    vector<int> ids(num_records);
    for (int i = 0; i < num_records; ++i) {
        ids[i] = i + 1;
    }

    random_device rd;
    mt19937 generator(rd());
    shuffle(ids.begin(), ids.end(), generator);

    ofstream out_file(config.dataset_file);
    if (!out_file.is_open()) {
        cerr << "Error: Could not create " << config.dataset_file << "!\n";
        return 1;
    }

    out_file << "ID,Payload\n";

    for (int i = 0; i < num_records; ++i) {
        string payload = "DummyData_Payload_For_ID_" + to_string(ids[i]);
        if (payload.length() < 59) {
            payload.append(59 - payload.length(), 'X');
        }

        out_file << ids[i] << "," << payload << "\n";
    }

    out_file.close();
    cout << "Successfully created '" << config.dataset_file << "'!" << endl;
    return 0;
}
