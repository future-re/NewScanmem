# memseek MCP bridge

`memseek_server.py` is a dependency-free stdio MCP server independent of the
C++ module build. Configure an MCP client with:

```json
{"mcpServers":{"memseek":{"command":"python3","args":["/absolute/path/to/Memseek/mcp/memseek_server.py"]}}}
```

Tools: `list_processes`, `inspect_maps`, `read_memory`, `scan_numeric`, and
`write_memory`. Writes require `NEWS_CAN_WRITE=1` and `confirm: true`.
