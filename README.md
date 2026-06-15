# LAB 1

## Small Test Config

The project reads runtime settings from `config.txt`. The default config is set up for a small local test:

```txt
dataset_file=dataset_small.csv
generated_record_count=1000
query_count=10
data_sizes=100,500,1000
range_threshold=2000
range_small=50
range_large=100
index_prefix=index_
```

These values avoid generating the full 10,000,000-row dataset while debugging.

## Manual Small Run

Build and run the data generator:

```bash
g++ data_generator.cpp -o data_generator.exe
./data_generator.exe
```

This creates the configured dataset file, usually `dataset_small.csv`.

Build the main program:

```bash
g++ Solution/main.cpp Solution/bplus_tree.cpp -o main.exe
```

Run the main program:

```bash
./main.exe
```

With the default small config, the program should read `dataset_small.csv`, load 10 query IDs, and iterate over data sizes `100`, `500`, and `1000`.

## Automated Smoke Test

Run the smoke test script:

```bash
tests/smoke_pipeline.sh
```

The script temporarily replaces `config.txt`, generates a 25-record dataset, builds both programs, runs `main`, checks the expected output, and restores the original config.

Expected final output:

```txt
PASS: smoke pipeline completed successfully.
```
## Unit Tests

```bash
tests/unit_node_io.sh
```

This test verifies that a fake leaf node can be written to offset 4096 and read back correctly.

## Final Benchmark

For the final benchmark, increase `config.txt` values, for example:

```txt
dataset_file=dataset_large.csv
generated_record_count=10000000
query_count=100
data_sizes=1000,3000,10000,30000,100000,300000,1000000,3000000,10000000
range_threshold=2000
range_small=500
range_large=1000
index_prefix=index_
```

Generated datasets, index files, executables, and object files are ignored by git.

Note: the current source still contains TODO sections for the B+ Tree insert/query implementation. The smoke test verifies the runnable pipeline skeleton; after implementing the tree logic, it can be extended to check real point and range query results.

## Git Workflow

### Quy tắc đặt tên branch

```
feature/<tên-tính-năng>     # thêm tính năng mới
fix/<tên-lỗi>               # sửa bug
chore/<công-việc>           # cập nhật config, docs,...
```

Ví dụ: `feature/data-preprocessing`, `fix/model-output-error`

### Các bước làm việc

```bash
# 1. Luôn cập nhật branch main trước
git checkout main
git pull origin main

# 2. Tạo branch mới từ main
git checkout -b feature/<tên-tính-năng>

# 3. Làm việc, sau đó commit
git add .
git commit -m "feat: mô tả ngắn thay đổi"

# 4. Push branch lên remote
git push origin feature/<tên-tính-năng>

# 5. Tạo Pull Request trên GitHub để merge vào main
```

### Quy tắc commit message

| Prefix | Dùng khi |
|--------|----------|
| `feat:` | Thêm tính năng mới |
| `fix:` | Sửa bug |
| `docs:` | Cập nhật tài liệu |
| `chore:` | Thay đổi config, dependencies |
| `refactor:` | Refactor code |

---