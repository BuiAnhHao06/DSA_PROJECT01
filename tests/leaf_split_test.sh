#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TEST_BIN="/tmp/dsa_project01_leaf_split_test"

g++ tests/leaf_split_test.cpp Solution/bplus_tree.cpp -I Solution -o "$TEST_BIN"
"$TEST_BIN"
