#!/usr/bin/env python3
"""
通用集成测试脚本：测试扫描和修改各种类型的值
启动目标进程，使用 NewScanmem 扫描并修改其内存中的值

用法:
    python3 test_integration.py <target_name> <value_type> <expected_value>

示例:
    python3 test_integration.py target_fixed_int int64 1122334455667788
    python3 test_integration.py target_fixed_double FLOAT_64 12345.6789
"""

import subprocess
import sys
import time
import re
import os
from pathlib import Path
import argparse


def strip_ansi_codes(text: str) -> str:
    """移除 ANSI 颜色控制码"""
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)


def parse_list_match_indices(output: str):
    """解析 list 输出中所有匹配项的索引（不筛选区域，按顺序全部尝试）"""
    clean_output = strip_ansi_codes(output)
    # 兼容 list 输出：示例格式
    # 0      0x00005f432f5b2088  0x4c      exe:.../target_fixed_int
    # 1      0x00007ffd3c2f1000  0x08      heap:...
    # 2      0x0000555555557000  0x08      stack:...
    pattern = r'^\s*(\d+)\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+'
    indices = []
    for line in clean_output.split('\n'):
        m = re.match(pattern, line)
        if m:
            indices.append(int(m.group(1)))
    return indices


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
    parser.add_argument('value_type', help='Value type to scan (e.g., int64, FLOAT_64)')
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
        # 先扫描，然后通过 list 查看所有匹配，最后写入到 exe 区域的第一个匹配
        commands = [
            f"pid {pid}",
            f"scan {args.value_type} = {args.expected_value}",
            "list",  # 列出所有匹配，用于找到 exe 区域的索引
        ]
        
        # 执行 NewScanmem 获取匹配列表
        print(f"[3] Executing scan commands...")
        
        scanmem_cmd = [str(scanmem_exe)]
        
        # 设置环境变量，禁用 ANSI 颜色输出
        env = os.environ.copy()
        env['NO_COLOR'] = '1'
        env['TERM'] = 'dumb'
        
        list_result = subprocess.run(
            scanmem_cmd,
            input="\n".join(commands) + "\nquit\n",
            capture_output=True,
            text=True,
            timeout=10,
            env=env
        )
        
        # 解析输出，获取所有匹配索引（不筛选 exe）
        match_indices = parse_list_match_indices(list_result.stderr)
        if not match_indices:
            print("[4] Warning: No matches found in list output", file=sys.stderr)
            match_indices = [0]
        else:
            print(f"[4] Found {len(match_indices)} matches: {match_indices}")

        # 逐个尝试写入，直到目标进程检测到修改或全部尝试完成
        result = 1
        for idx in match_indices:
            if target_proc.poll() is not None:
                # 进程已退出，判断结果
                exit_code = target_proc.returncode
                if exit_code == 0:
                    print(f"[5] Target process already exited successfully (exit code: {exit_code})")
                    result = 0
                else:
                    print(f"[5] Target process already exited with error (exit code: {exit_code})", file=sys.stderr)
                break

            write_commands = [
                f"pid {pid}",
                f"scan {args.value_type} = {args.expected_value}",
                f"write {args.modified_value} {idx}",
            ]

            print(f"[5] Trying write on match index {idx}...")
            for cmd in write_commands:
                print(f"    > {cmd}")

            env = os.environ.copy()
            env['NO_COLOR'] = '1'
            env['TERM'] = 'dumb'

            scanmem_result = subprocess.run(
                scanmem_cmd,
                input="\n".join(write_commands) + "\nquit\n",
                capture_output=True,
                text=True,
                timeout=10,
                env=env
            )

            if scanmem_result.returncode != 0:
                print(f"[6] NewScanmem failed (index {idx}) with exit code {scanmem_result.returncode}", file=sys.stderr)
                print(f"STDOUT:\n{scanmem_result.stdout}", file=sys.stderr)
                print(f"STDERR:\n{scanmem_result.stderr}", file=sys.stderr)
                # 失败则继续尝试下一个索引
                continue
            else:
                print(f"[6] NewScanmem executed successfully for index {idx}")
                stdout_clean = strip_ansi_codes(scanmem_result.stdout)
                stderr_clean = strip_ansi_codes(scanmem_result.stderr)
                if stdout_clean.strip():
                    print(f"STDOUT:\n{stdout_clean}")
                if stderr_clean.strip():
                    print(f"STDERR:\n{stderr_clean}")

                # 写入后短暂等待，查看目标进程是否因检测成功而退出
                print(f"\n[7] Waiting briefly for target validation after index {idx}...")
                try:
                    exit_code = target_proc.wait(timeout=min(2.0, args.wait_modify_ms / 1000))
                    if exit_code == 0:
                        print(f"[8] Target process exited successfully (exit code: {exit_code})")
                        print("    ✓ Value was successfully modified and detected!")
                        result = 0
                        break
                    else:
                        print(f"[8] Target process exited with error (exit code: {exit_code})", file=sys.stderr)
                        stdout, stderr = target_proc.communicate()
                        if stdout:
                            print(f"Target stdout:\n{stdout}", file=sys.stderr)
                        if stderr:
                            print(f"Target stderr:\n{stderr}", file=sys.stderr)
                        # 错误退出，停止进一步尝试
                        result = 1
                        break
                except subprocess.TimeoutExpired:
                    # 目标进程仍在运行，继续尝试下一个匹配
                    print(f"[8] Target still running after index {idx}, continue...")

        # 若所有尝试后仍未成功，则等待到总超时或直接判定失败
        if result != 0 and target_proc.poll() is None:
            print(f"\n[9] All writes attempted, waiting until timeout for final check...")
            try:
                exit_code = target_proc.wait(timeout=args.wait_modify_ms / 1000)
                if exit_code == 0:
                    print(f"[9] Target process exited successfully (exit code: {exit_code})")
                    print("    ✓ Value was successfully modified and detected!")
                    result = 0
                else:
                    print(f"[9] Target process exited with error (exit code: {exit_code})", file=sys.stderr)
                    stdout, stderr = target_proc.communicate()
                    if stdout:
                        print(f"Target stdout:\n{stdout}", file=sys.stderr)
                    if stderr:
                        print(f"Target stderr:\n{stderr}", file=sys.stderr)
                    result = 1
            except subprocess.TimeoutExpired:
                print(f"[9] Target process did not exit within expected time", file=sys.stderr)
                result = 1
            
    finally:
        # 清理：确保目标进程被终止
        print("\n[10] Cleaning up...")
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
