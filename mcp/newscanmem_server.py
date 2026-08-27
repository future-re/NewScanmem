#!/usr/bin/env python3
"""Dependency-free stdio MCP bridge for safe Linux memory inspection."""
from __future__ import annotations

import json
import os
import re
import struct
import sys
from pathlib import Path
from typing import Any

MAX_READ = 1024 * 1024
MAX_SCAN_BYTES = 64 * 1024 * 1024
MAX_MATCHES = 1000
MAP_LINE = re.compile(r"^([0-9a-f]+)-([0-9a-f]+)\s+([-rwxps]{4})\s+([0-9a-f]+)\s+([^\s]+)\s+(\d+)(?:\s+(.*))?$")


class ToolError(Exception):
    pass


def pid_of(value: Any) -> int:
    try:
        pid = int(value)
    except (TypeError, ValueError) as exc:
        raise ToolError("pid must be an integer") from exc
    if pid <= 0 or not Path(f"/proc/{pid}").is_dir():
        raise ToolError(f"process {pid} does not exist")
    return pid


def check_owner(pid: int) -> None:
    if os.geteuid() == 0:
        return
    try:
        status = Path(f"/proc/{pid}/status").read_text(encoding="ascii")
    except OSError as exc:
        raise ToolError(f"cannot inspect process {pid}: {exc.strerror}") from exc
    line = next((x for x in status.splitlines() if x.startswith("Uid:")), "")
    if not line or int(line.split()[1]) != os.getuid():
        raise ToolError("access denied: target is owned by another user")


def maps_for(value: Any, writable: bool = False) -> list[dict[str, Any]]:
    pid = pid_of(value)
    check_owner(pid)
    try:
        lines = Path(f"/proc/{pid}/maps").read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"cannot read maps: {exc.strerror}") from exc
    result = []
    for line in lines:
        m = MAP_LINE.match(line)
        if not m:
            continue
        start, end = int(m.group(1), 16), int(m.group(2), 16)
        perms = m.group(3)
        if "r" not in perms or (writable and "w" not in perms):
            continue
        result.append({"start": start, "end": end, "size": end - start,
                       "permissions": perms, "offset": int(m.group(4), 16),
                       "device": m.group(5), "inode": int(m.group(6)),
                       "pathname": m.group(7) or ""})
    return result


def containing(mapping: list[dict[str, Any]], address: int, length: int) -> dict[str, Any]:
    if address < 0 or length <= 0 or length > MAX_READ:
        raise ToolError(f"length must be between 1 and {MAX_READ}")
    end = address + length
    for region in mapping:
        if region["start"] <= address and end <= region["end"]:
            return region
    raise ToolError("requested range is not contained in one readable mapping")


def read_bytes(pid: int, address: int, length: int) -> bytes:
    region = containing(maps_for(pid), address, length)
    del region
    try:
        fd = os.open(f"/proc/{pid}/mem", os.O_RDONLY | os.O_CLOEXEC)
        try:
            data = os.pread(fd, length, address)
        finally:
            os.close(fd)
    except OSError as exc:
        raise ToolError(f"memory read failed: {exc.strerror}") from exc
    if len(data) != length:
        raise ToolError(f"short read: requested {length}, received {len(data)}")
    return data


def write_bytes(pid: int, address: int, data: bytes, confirm: bool) -> dict[str, Any]:
    if os.environ.get("NEWS_CAN_WRITE") != "1":
        raise ToolError("writes are disabled; set NEWS_CAN_WRITE=1 for explicit opt-in")
    if not confirm:
        raise ToolError("write requires confirm=true")
    containing(maps_for(pid, writable=True), address, len(data))
    try:
        fd = os.open(f"/proc/{pid}/mem", os.O_RDWR | os.O_CLOEXEC)
        try:
            written = os.pwrite(fd, data, address)
        finally:
            os.close(fd)
    except OSError as exc:
        raise ToolError(f"memory write failed: {exc.strerror}") from exc
    if written != len(data):
        raise ToolError(f"short write: expected {len(data)}, wrote {written}")
    return {"address": address, "bytes_written": written}


def numeric_format(dtype: str, endian: str) -> tuple[str, int]:
    formats = {"int8": "b", "uint8": "B", "int16": "h", "uint16": "H",
               "int32": "i", "uint32": "I", "int64": "q", "uint64": "Q",
               "float32": "f", "float64": "d"}
    if dtype not in formats:
        raise ToolError("unsupported dtype: int8/16/32/64, uint8/16/32/64, float32, float64")
    return (">" if endian == "big" else "<") + formats[dtype], struct.calcsize(formats[dtype])


def scan_numeric(args: dict[str, Any]) -> dict[str, Any]:
    pid = pid_of(args.get("pid"))
    fmt, width = numeric_format(str(args.get("dtype", "int32")).lower(), str(args.get("endianness", "little")).lower())
    if "value" not in args:
        raise ToolError("value is required")
    step = max(int(args.get("step", 1)), 1)
    limit = min(max(int(args.get("max_matches", 100)), 1), MAX_MATCHES)
    found, scanned = [], 0
    for region in maps_for(pid):
        if scanned >= MAX_SCAN_BYTES:
            break
        size = min(region["size"], MAX_SCAN_BYTES - scanned)
        try:
            data = read_bytes(pid, region["start"], size)
        except ToolError:
            continue
        scanned += len(data)
        for offset in range(0, len(data) - width + 1, step):
            value = struct.unpack_from(fmt, data, offset)[0]
            if value == args["value"]:
                found.append({"address": region["start"] + offset, "value": value, "region": region["pathname"]})
                if len(found) == limit:
                    return {"matches": found, "match_count": len(found), "bytes_scanned": scanned, "truncated": True}
    return {"matches": found, "match_count": len(found), "bytes_scanned": scanned, "truncated": False}


def list_processes() -> list[dict[str, Any]]:
    result = []
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            stat = (entry / "stat").read_text(encoding="utf-8")
            name = stat[stat.find("(") + 1:stat.rfind(")")]
            cmdline = (entry / "cmdline").read_bytes().replace(b"\0", b" ").decode(errors="replace").strip()
            result.append({"pid": int(entry.name), "name": name, "cmdline": cmdline})
        except (OSError, ValueError):
            continue
    return sorted(result, key=lambda x: x["pid"])


TOOLS = [
    {"name": "list_processes", "description": "List visible Linux processes.", "inputSchema": {"type": "object", "properties": {}}},
    {"name": "inspect_maps", "description": "Inspect readable mappings for a process.", "inputSchema": {"type": "object", "required": ["pid"], "properties": {"pid": {"type": "integer"}, "writable_only": {"type": "boolean"}}}},
    {"name": "read_memory", "description": "Read a bounded range as hexadecimal bytes.", "inputSchema": {"type": "object", "required": ["pid", "address", "length"], "properties": {"pid": {"type": "integer"}, "address": {"type": "integer"}, "length": {"type": "integer"}}}},
    {"name": "scan_numeric", "description": "Scan readable mappings for an exact numeric value.", "inputSchema": {"type": "object", "required": ["pid", "dtype", "value"], "properties": {"pid": {"type": "integer"}, "dtype": {"type": "string"}, "value": {}, "endianness": {"type": "string"}, "step": {"type": "integer"}, "max_matches": {"type": "integer"}}}},
    {"name": "write_memory", "description": "Write bytes only with explicit opt-in and confirmation.", "inputSchema": {"type": "object", "required": ["pid", "address", "hex", "confirm"], "properties": {"pid": {"type": "integer"}, "address": {"type": "integer"}, "hex": {"type": "string"}, "confirm": {"type": "boolean"}}}},
]


def call_tool(name: str, args: dict[str, Any]) -> Any:
    if name == "list_processes":
        return {"processes": list_processes()}
    if name == "inspect_maps":
        return {"mappings": maps_for(args.get("pid"), bool(args.get("writable_only", False)))}
    if name == "read_memory":
        address, length = int(args["address"]), int(args["length"])
        return {"address": address, "length": length, "hex": read_bytes(pid_of(args.get("pid")), address, length).hex()}
    if name == "scan_numeric":
        return scan_numeric(args)
    if name == "write_memory":
        try:
            data = bytes.fromhex(str(args.get("hex", "")))
        except ValueError as exc:
            raise ToolError("hex must contain an even number of hexadecimal digits") from exc
        if not data or len(data) > MAX_READ:
            raise ToolError(f"hex payload must be between 1 and {MAX_READ} bytes")
        pid = pid_of(args.get("pid"))
        return write_bytes(pid, int(args["address"]), data, bool(args.get("confirm", False)))
    raise ToolError(f"unknown tool: {name}")


def send(request_id: Any, result: Any = None, error: str | None = None) -> None:
    response: dict[str, Any] = {"jsonrpc": "2.0", "id": request_id}
    if error is not None:
        response["error"] = {"code": -32000, "message": error}
    else:
        response["result"] = result
    sys.stdout.write(json.dumps(response, separators=(",", ":")) + "\n")
    sys.stdout.flush()


def main() -> int:
    for line in sys.stdin:
        try:
            request = json.loads(line)
            method, request_id = request.get("method"), request.get("id")
            if method == "initialize":
                send(request_id, {"protocolVersion": "2024-11-05", "capabilities": {"tools": {}}, "serverInfo": {"name": "newscanmem", "version": "0.1.0"}})
            elif method == "notifications/initialized":
                continue
            elif method == "tools/list":
                send(request_id, {"tools": TOOLS})
            elif method == "tools/call":
                params = request.get("params", {})
                try:
                    value = call_tool(str(params.get("name")), params.get("arguments", {}))
                    send(request_id, {"content": [{"type": "text", "text": json.dumps(value, ensure_ascii=False)}], "structuredContent": value})
                except (ToolError, KeyError, TypeError, ValueError) as exc:
                    send(request_id, error=str(exc))
            else:
                send(request_id, error=f"unsupported method: {method}")
        except json.JSONDecodeError as exc:
            send(None, error=f"invalid JSON: {exc.msg}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
