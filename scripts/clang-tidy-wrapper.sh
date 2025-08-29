#!/usr/bin/env bash
CLANG_TIDY=clang-tidy

# 默认补充的编译器参数（使用 --extra-arg=... 形式）
DEFAULT_COMPILER_ARGS="--extra-arg=-std=c++23 --extra-arg=-fmodules --extra-arg=-fimplicit-modules --extra-arg=-Isrc --extra-arg-before=--driver-mode=g++"

# 如果调用参数中已经包含 "--"，直接原样转发
for a in "$@"; do
  if [ "$a" = "--" ]; then
    exec "$CLANG_TIDY" --checks='*' --enable-module-headers-parsing "$@"
  fi
done

# 否则在末尾追加 -- 和默认编译器参数
exec "$CLANG_TIDY" --checks='*' --enable-module-headers-parsing "$@" -- $DEFAULT_COMPILER_ARGS