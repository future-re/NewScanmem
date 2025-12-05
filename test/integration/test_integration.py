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
    parser.add_argument('modified_value', help='Value to write into target process')
    parser.add_argument('--wait-modify-ms', type=int, default=10000, 
                       help='Timeout for waiting modification (default: 10000ms)')
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
    print(f"Modified value: {args.modified_value}")
    
    # 启动目标进程
    print("\n[1] Starting target process...")
    target_proc = subprocess.Popen(
        [str(target_exe), "--duration-ms", "30000", 
         "--wait-modify-ms", str(args.wait_modify_ms)],  # 传递超时参数
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
        # 使用 NewScanmem 进行扫描和修改
        print(f"\n[2] Scanning process {pid} for {args.value_type} value {args.expected_value}")
        
        # 准备命令序列
        commands = [
            f"pid {pid}",  # 附加到目标进程
            f"scan {args.value_type} = {args.expected_value}",  # 扫描期望值
            f"write {args.modified_value} 0",  # 写入新值到第一个匹配
        ]
        
        # 将命令通过 stdin 发送给 NewScanmem
        command_input = "\n".join(commands) + "\nquit\n"
        
        # 执行 NewScanmem
        print(f"[3] Executing commands:")
        for cmd in commands:
            print(f"    > {cmd}")
        
        # NewScanmem 需要 root 权限来 attach 到进程
        scanmem_cmd = ["sudo", str(scanmem_exe)]
        
        scanmem_result = subprocess.run(
            scanmem_cmd,
            input=command_input,
            capture_output=True,
            text=True,
            timeout=10
        )
        
        # 检查 NewScanmem 执行结果
        if scanmem_result.returncode != 0:
            print(f"[4] NewScanmem failed with exit code {scanmem_result.returncode}", file=sys.stderr)
            print(f"STDOUT:\n{scanmem_result.stdout}", file=sys.stderr)
            print(f"STDERR:\n{scanmem_result.stderr}", file=sys.stderr)
            result = 1
        else:
            print("[4] NewScanmem executed successfully")
            print(f"STDOUT:\n{scanmem_result.stdout}")
            if scanmem_result.stderr:
                print(f"STDERR:\n{scanmem_result.stderr}")
            
            # 等待目标进程结束并检查退出码
            print(f"\n[5] Waiting for target process to validate modification...")
            try:
                exit_code = target_proc.wait(timeout=args.wait_modify_ms / 1000 + 5)
                if exit_code == 0:
                    print(f"[6] Target process exited successfully (exit code: {exit_code})")
                    print("    ✓ Value was successfully modified and detected!")
                    result = 0
                else:
                    print(f"[6] Target process exited with error (exit code: {exit_code})", file=sys.stderr)
                    print("    ✗ Value modification was not detected within timeout", file=sys.stderr)
                    # 打印目标进程输出以便调试
                    stdout, stderr = target_proc.communicate()
                    if stdout:
                        print(f"Target stdout:\n{stdout}", file=sys.stderr)
                    if stderr:
                        print(f"Target stderr:\n{stderr}", file=sys.stderr)
                    result = 1
            except subprocess.TimeoutExpired:
                print(f"[6] Target process did not exit within expected time", file=sys.stderr)
                result = 1
            
    finally:
        # 清理：确保目标进程被终止
        print("\n[7] Cleaning up...")
        if target_proc.poll() is None:
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
