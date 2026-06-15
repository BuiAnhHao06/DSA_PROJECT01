#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TEST_BIN="/tmp/dsa_project01_unit_node_io"

g++ tests/unit_node_io.cpp Solution/bplus_tree.cpp -I Solution -o "$TEST_BIN"
"$TEST_BIN"
