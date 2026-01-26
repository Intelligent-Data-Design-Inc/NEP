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

## GitHub Interactions
- **Prefer GitHub CLI**: Use `gh` command line tool instead of MCP GitHub tools
- **MCP GitHub tools**: Unreliable due to TLS handshake timeouts on slow network
- **Issue creation**: Use `gh issue create --title "..." --body "..."` or `gh issue create --body-file <file>`
- **PR operations**: Use `gh pr create`, `gh pr view`, `gh pr comment`, etc.
- **Authentication**: Ensure `gh auth status` shows valid credentials before operations
- **Fallback**: If `gh` unavailable, document the action needed and ask user to perform manually
