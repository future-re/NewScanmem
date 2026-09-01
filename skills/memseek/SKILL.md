---
name: memseek
description: Use memseek MCP tools for constrained Linux process-memory inspection and evidence-based scans.
---

Use `list_processes`, `inspect_maps`, `read_memory`, and `scan_numeric` for
read-only analysis. Treat mappings and addresses as volatile; inspect maps
before reads and report short-read or permission failures.

Never enable `NEWS_CAN_WRITE` or call `write_memory` unless the user explicitly
authorizes the PID, address, byte payload, and reason. Writes require
`confirm=true`. Verify writable permissions and report the exact bytes written.

For reverse engineering, corroborate observations with mapping permissions and
neighboring bytes. Do not execute target code or attach a debugger through this
skill.
