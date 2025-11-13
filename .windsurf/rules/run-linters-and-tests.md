---
trigger: manual
---

# Lint and Test Rule

- **[scope]** Whenever modifying backend code within `backend/` or frontend code within `frontend/`, Cascade must rerun the relevant linters and tests before marking the task complete.
- **[backend]** Execute `flake8` and `pytest` (or appropriately scoped test commands when changes are limited).
- **[frontend]** Execute `npm run lint` and `npm test` (or the project’s equivalent scripts).
- **[documentation]** Record the commands executed and confirm successful completion in the task summary or final handoff.