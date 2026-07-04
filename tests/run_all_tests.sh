#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TESTS=(
    "tests/search_test.sh"
    "tests/unit_node_io.sh"
    "tests/header_test.sh"
    "tests/insert_leaf_test.sh"
    "tests/leaf_split_test.sh"
    "tests/internal_insert_test.sh"
    "tests/internal_split_test.sh"
    "tests/tree_validate_test.sh"
    "tests/point_query_test.sh"
    "tests/point_query_compare_test.sh"
    "tests/range_query_bplus_test.sh"
    "tests/range_query_compare_test.sh"
    "tests/benchmark_small_test.sh"
    "tests/smoke_pipeline.sh"
)

for test_script in "${TESTS[@]}"; do
    echo "============================================================"
    echo "RUNNING: $test_script"
    echo "============================================================"
    "$test_script"
done

echo "============================================================"
echo "PASS: all tests completed successfully."
