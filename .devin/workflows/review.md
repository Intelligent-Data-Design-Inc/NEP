---
description: Review pull request and add code review comments
---

# Pull Request Code Review Workflow

This workflow performs a comprehensive code review on a pull request and adds review comments directly to the PR.

## Steps

1. **Fetch pull request details**
   - Get PR information using the pull request number
   - Retrieve the list of changed files
   - Get the diff for each changed file
   - Identify added, modified, and deleted lines

2. **Perform comprehensive code review**
   - Analyze code quality and adherence to best practices
   - Identify potential bugs or edge cases
   - Look for performance optimization opportunities
   - Assess readability and maintainability
   - Check for security concerns
   - Review documentation completeness
   - Check for code duplication
   - Verify error handling patterns
   - Ensure unit tests are included for new/modified code

3. **Generate review comments**
   - Create a list of code review comments for each issue found
   - For each comment, include:
     - **File**: The file being commented on
     - **Line**: Specific line number (or range)
     - **Type**: Suggestion/Issue/Question/Praise
     - **Comment**: Detailed feedback or question
     - **Severity**: Critical/High/Medium/Low

4. **Add review comments to PR**
   - For each review comment, ask: "Add this comment to PR? (yes/no)"
   - If user says **yes**: Add the comment as a PR review comment using the appropriate line
   - If user says **no**: Skip and move to next comment
   - Continue until all comments are processed

5. **Submit overall review**
   - Ask user for overall review status: "Approve", "Request Changes", or "Comment"
   - Add summary comment with overall assessment
   - Submit the review with the chosen status

6. **Summary**
   - Report how many comments were added
   - List the types of issues found
   - Note the overall review status

## Usage

```
/review <pull_request_number>
```

Example:
```
/review 123
```

## Notes

- Comments are added as pull request review comments on specific lines
- Use appropriate comment types (suggestion, issue, question, praise)
- Include code references using GitHub's line selection
- Review focuses on code quality, not creating separate issues
- For critical issues that need separate tracking, suggest creating issues in comments
- Group related issues when appropriate 