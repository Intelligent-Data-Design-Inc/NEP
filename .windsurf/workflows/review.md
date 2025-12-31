---
description: Review code and create GitHub issues
---

# Code Review Workflow

This workflow performs a comprehensive code review and generates actionable GitHub issues.

## Steps

1. **Perform comprehensive code review**
   - Analyze code quality and adherence to best practices
   - Identify potential bugs or edge cases
   - Look for performance optimization opportunities
   - Assess readability and maintainability
   - Check for security concerns
   - Review documentation completeness
   - Check for code duplication
   - Verify error handling patterns

2. **Generate issue recommendations**
   - Create a numbered list of potential GitHub issues
   - For each issue, include:
     - **Title**: Clear, concise issue title
     - **Priority**: Critical/High/Medium/Low
     - **Type**: Bug/Enhancement/Refactor/Documentation/Security
     - **Description**: Detailed explanation of the issue
     - **Location**: Specific file(s) and line numbers
     - **Suggested Fix**: How to address the issue
   - Present all issues to the user for approval

3. **Interactive issue creation**
   - For each recommended issue, ask: "Create this issue? (yes/no)"
   - If user says **yes**: Create the GitHub issue with all details
   - If user says **no**: Skip and move to next issue
   - Continue until all issues are processed

4. **Summary**
   - Report how many issues were created
   - List the issue numbers and titles created

## Usage

```
/review <file_path>
```

Example:
```
/review src/geotifffile.c
```

## Notes

- Issues are created in the current repository
- Use appropriate labels based on issue type
- Include code references using GitHub's file:line syntax
- Group related issues when appropriate 