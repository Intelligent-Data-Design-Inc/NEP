---
trigger: always_on
---

# Network Resilience Guidelines

The network here is slow and flakey. Follow these specific retry strategies:

## HTTP Requests (read_url_content, search_web)
- **Retry count**: 3 attempts
- **Timeout**: 10 seconds per attempt
- **Backoff**: 5 seconds between retries
- **Pre-check**: Use `curl -I` for connectivity before full requests

## MCP Server Calls
- **Retry count**: 5 attempts
- **Delay**: 2 seconds wait between attempts
- **Timeout**: 30 seconds per call
- **Pre-check**: Verify MCP server process is running

## Git Operations
- **Retry count**: Immediate retry once
- **Issue**: Distinguish network vs authentication failures
- **Auth failures**: Don't retry - check credentials

## Diagnostic Commands
- **Basic connectivity**: `ping -c 1 8.8.8.8`
- **HTTP test**: `curl -I https://github.com`
- **MCP status**: Check process list for MCP server

## Failure Type Handling
- **Timeouts**: Retry with exponential backoff
- **Connection refused**: Check if service is running
- **Authentication errors**: Don't retry - fix credentials first
- **DNS failures**: Check `/etc/resolv.conf` and retry

## Tool-Specific Guidance
- **bash network commands**: Always test connectivity first
- **file operations**: Local only - no retries needed
- **build commands**: Network-dependent parts need retry logic
