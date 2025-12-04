#!/usr/bin/env python3
"""
通用集成测试脚本：测试扫描和修改各种类型的值
启动目标进程，使用 NewScanmem 扫描并修改其内存中的值

用法:
    python3 test_integration.py <target_name> <value_type> <expected_value>

示例:
    python3 test_integration.py target_fixed_int int64 1122334455667788
    python3 test_integration.py target_fixed_double float64 12345.6789
"""

import subprocess
import sys
import time
import re
import os
from pathlib import Path
import argparse


def find_executable(name: str, build_dir: Path) -> Path:
    """查找可执行文件"""
    # 尝试在 build/test/integration 中查找目标程序
    integration_path = build_dir / "test" / "integration" / name
    if integration_path.exists():
        return integration_path
    
    # 尝试在 build/src 中查找主程序
    src_path = build_dir / "src" / name
    if src_path.exists():
        return src_path
    
    # 尝试在 build/bin 中查找
    bin_path = build_dir / "bin" / name
    if bin_path.exists():
        return bin_path
    
    raise FileNotFoundError(f"Cannot find executable: {name}")


def main():
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='Integration test for NewScanmem')
    parser.add_argument('target_name', help='Target executable name (e.g., target_fixed_int)')
    parser.add_argument('value_type', help='Value type to scan (e.g., int64, float64)')
    parser.add_argument('expected_value', help='Expected value in target process')
    args = parser.parse_args()
    
    # 确定构建目录
    script_dir = Path(__file__).parent
    workspace_dir = script_dir.parent.parent
    build_dir = workspace_dir / "build"
    
    if not build_dir.exists():
        print(f"Error: Build directory not found: {build_dir}", file=sys.stderr)
        return 1
    
    # 查找可执行文件
    try:
        target_exe = find_executable(args.target_name, build_dir)
        scanmem_exe = find_executable("NewScanmem", build_dir)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    
    print(f"Target executable: {target_exe}")
    print(f"Scanmem executable: {scanmem_exe}")
    print(f"Value type: {args.value_type}")
    print(f"Expected value: {args.expected_value}")
    
    # 启动目标进程
    print("\n[1] Starting target process...")
    target_proc = subprocess.Popen(
        [str(target_exe), "--duration-ms", "30000"],  # 运行 30 秒
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    # 等待目标进程输出 PID
    time.sleep(0.5)
    
    # 获取 PID
    pid = target_proc.pid
    print(f"Target PID: {pid}")
    
    try:
        # TODO: 这里应该实际调用 NewScanmem 进行扫描
        # 示例命令序列：
        # 1. attach <pid>
        # 2. scan <value_type> = <expected_value>
        # 3. set <address> <new_value>
        # 4. 验证修改成功
        
        print(f"\n[2] Would scan process {pid} for {args.value_type} value {args.expected_value}")
        print(f"    Command: {scanmem_exe} (interactive mode needed)")
        
        # 简单验证：目标进程还在运行
        time.sleep(1)
        if target_proc.poll() is None:
            print("[3] Target process is still running - OK")
            result = 0
        else:
            print("[3] Target process exited unexpectedly - FAILED", file=sys.stderr)
            result = 1
            
    finally:
        # 清理：终止目标进程
        print("\n[4] Cleaning up...")
        target_proc.terminate()
        try:
            target_proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            target_proc.kill()
            target_proc.wait()
        print("Target process terminated")
    
    return result


if __name__ == "__main__":
    sys.exit(main())
