#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TEST_BIN="/tmp/dsa_project01_internal_insert_test"

g++ tests/internal_insert_test.cpp Solution/bplus_tree.cpp -I Solution -o "$TEST_BIN"
"$TEST_BIN"
