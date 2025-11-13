---
description: implement a sprint
auto_execution_mode: 3
---

Implement the sprint plan for [version] sprint [number] following these steps:

1. Read the sprint plan from docs/plan/[version]-sprint[number].md
2. Review related context:
   - docs/roadmap.md - Feature requirements and technical deliverables
   - docs/prd.md - Product requirements and functional specifications
   - docs/design.md - Architecture and technical design
3. Execute tasks in order following the plan's phase structure
4. For each task:
   - Complete all subtasks with checkboxes
   - Meet acceptance criteria before moving to next task
   - Update documentation as specified
   - Write tests achieving target coverage
5. Mark plan steps as complete using the task management system
6. Verify Definition of Done criteria before concluding
7. Run the /btest workflow to verify all tests pass

Implementation notes:
- Follow existing code patterns and architecture decisions documented in the plan
- Use the technical notes and code examples provided in each task
- Consult the risk assessment if you encounter blockers
- Reference the success metrics to validate your implementation

Example usage: "Implement v0.4.0 sprint 2"