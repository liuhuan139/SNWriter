#!/bin/bash
# 测试脚本逻辑

SOURCE="${BASH_SOURCE[0]}"
echo "BASH_SOURCE[0] = $SOURCE"

while [ -h "$SOURCE" ]; do
    DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
    echo "Resolved symlink to: $SOURCE"
done

SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
echo "Final SCRIPT_DIR = $SCRIPT_DIR"
echo "Final script path = $SOURCE"
