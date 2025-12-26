---
description: implement a sprint
auto_execution_mode: 3
---

Implement the work defined in GitHub issue [issue_number] following these steps:

1. Fetch the GitHub issue using the MCP GitHub server to retrieve issue details
2. Extract the implementation plan from the issue body and comments:
   - Look for "Technical Implementation Plan" or "Implementation Roadmap" sections
   - Identify phases, tasks, and acceptance criteria
   - Note any technical specifications and code examples
3. Review related context for architecture and design patterns:
   - docs/design.md - Architecture and technical design
   - docs/prd.md - Product requirements and functional specifications
4. Execute tasks in order following the plan's phase structure
5. For each task:
   - Complete all subtasks with checkboxes
   - Meet acceptance criteria before moving to next task
   - Update documentation as specified
   - Write tests achieving target coverage
6. Mark plan steps as complete using the task management system
7. Verify Definition of Done criteria before concluding
8. Run the /btest workflow to verify all tests pass

Implementation notes:
- Follow existing code patterns and architecture decisions documented in the issue
- Use the technical notes and code examples provided in each task
- Consult the issue comments for any clarifications or updates
- Reference the success metrics to validate your implementation
- Post progress updates to the GitHub issue as needed

Example usage: "/implement 60" or "Implement issue 60"