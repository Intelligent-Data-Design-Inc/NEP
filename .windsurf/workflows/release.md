---
description: Generate GitHub release notes from issues and projects
auto_execution_mode: 3
---

Generate professional GitHub release notes for a specific version by extracting information from GitHub issues, pull requests, and project boards, following the established release notes format.

## Prerequisites
1. Identify the version to document (user will specify, e.g., "0.5.0", "1.0.0")
2. Check `docs/releases/` for the latest release notes format examples (v0.4.0.md is comprehensive)
3. Determine the GitHub milestone or project board associated with this version
4. Get GitHub repository details (owner: Intelligent-Data-Design-Inc, repo: NEP)

## Steps

### 1. Gather Issues and PRs for Version
Use GitHub MCP tools to collect:
- **Closed issues** in the version milestone (if milestone exists)
- **Merged pull requests** associated with the version
- **Project board items** (if using GitHub Projects for version tracking)
- **Release date**: Use the date of the last merged PR or current date

Query strategy:
```
# Search for issues closed in version
mcp0_search_issues: query="milestone:v{VERSION} is:closed"

# Search for merged PRs in version  
mcp0_search_pull_requests: query="is:merged milestone:v{VERSION}"

# Alternative: Search by labels if no milestone
mcp0_search_issues: query="label:v{VERSION} is:closed"
```

### 2. Categorize Issues by Type
Analyze collected issues and PRs, grouping them by:
- **Features**: New functionality (label: enhancement, feature)
- **Bug Fixes**: Resolved issues (label: bug, fix)
- **Build System**: CMake, Autotools changes (label: build)
- **Testing**: Test coverage, new tests (label: testing)
- **Documentation**: Docs updates (label: documentation)
- **Dependencies**: New or updated libraries
- **API Changes**: NetCDF API extensions, dispatch changes
- **Performance**: Optimizations, benchmarks

Extract from each issue:
- Title and issue number
- Description/summary
- Implementation details from PR description
- Related files/components modified

### 3. Review Existing Release Notes Format
Examine the most recent release notes (e.g., `docs/releases/v0.4.0.md`) to understand:
- Section structure and order
- Writing style (professional, concise, technical)
- How features are organized by category
- Level of technical detail expected

### 4. Create Release Notes Structure
Generate a markdown file named `docs/releases/v{VERSION}.md` with these sections:

**Required Sections:**
- **Title & Subtitle**: Version number + main theme (derived from most impactful features), followed by `>` blockquote with one-line description
- **Highlights**: 4-6 bullet points of major achievements (start with bold emphasis, reference issue numbers)
- **Features**: Organized by functional areas with detailed descriptions
  - For each feature: "**[Feature Name]** (#123) - Description with implementation details"
  - Group by: UDF handlers, compression, build system, file format support, etc.
- **Bug Fixes**: List significant bugs resolved with issue numbers
  - Format: "**[Bug description]** (#456) - Resolution details"
- **API Changes**: New/modified NetCDF API extensions, NC_Dispatch implementations (if applicable)
- **Build System**: CMake and Autotools changes, new dependencies, configuration options
- **Testing**: Coverage metrics, test strategies, new test files added
- **Documentation**: Doxygen updates, API documentation, user guides (if applicable)
- **Dependencies Added**: New libraries (HDF5, NetCDF-C, NCEPLIBS-g2, etc.) with versions
- **Known Limitations**: Open issues or deferred work (link to GitHub issues)
- **Breaking Changes**: None or list with migration guidance

**Footer Section:**
- **Released**: Date (format: YYYY-MM-DD, use last merged PR date or current date)
- **Scope**: List GitHub milestone or project board name
- **Issues Closed**: Total count of issues/PRs included
- **Contributors**: GitHub usernames of PR authors (if multiple contributors)

### 5. Writing Guidelines
- **Be concise**: Keep total length under 100-120 lines (~1 page when rendered)
- **Use technical language**: Include file paths, function names, specific implementations
- **Reference issues**: Always include issue/PR numbers in format (#123)
- **Emphasize user impact**: Start features with user-facing benefits
- **Cross-reference docs**: Link to architecture docs, API specs, testing guides
- **Maintain consistency**: Match tone and structure of previous releases
- **Prioritize information**: Most important features first, details second
- **Credit contributors**: Mention PR authors when appropriate

### 6. Content Transformation Rules
When converting GitHub issues/PRs to release notes:
- **Issue titles → Feature descriptions**: Expand with context and implementation details from PR
- **PR descriptions → Implementation details**: Extract what was built, where in codebase, which files modified
- **Labels → Section assignment**: Use issue labels to categorize into appropriate sections
- **Closed issues → Highlights**: Select 4-6 most impactful closed issues for Highlights section
- **Open issues → Known Limitations**: Reference deferred or incomplete work
- **PR merge dates → Release date**: Use the latest merge date as release date
- **Multiple related issues → Single feature**: Combine related issues into cohesive feature descriptions

### 7. Quality Checks
Before finalizing:
- ✅ Version number matches milestone/project (format: v0.X.0 or v1.X.0)
- ✅ File name matches version (e.g., `v0.5.0.md` for Version 0.5.0)
- ✅ All issue numbers are valid and link correctly (#123 format)
- ✅ All sections present (even if "None" or "N/A")
- ✅ Length is under 100-120 lines
- ✅ No placeholder text like "[TODO]" or "[INSERT]"
- ✅ Technical terms are accurate (verify file paths, function names)
- ✅ Consistent formatting (bold for emphasis, code blocks for file paths)
- ✅ Issue/PR counts match actual GitHub data
- ✅ Contributors are credited appropriately

### 8. Finalization
- Save the file to `docs/releases/v{VERSION}.md`
- Do NOT commit or push (per user rules)
- Provide summary: "Release notes for v{VERSION} generated from {N} issues and {M} PRs"
- Inform the user the release notes are ready for review

## Example Usage

**Scenario 1: Version with Milestone**
```
User: "Create release notes for version 0.5.0"
→ Search GitHub for milestone:v0.5.0 issues and PRs
→ Categorize by labels (enhancement, bug, build, etc.)
→ Generate `docs/releases/v0.5.0.md`
→ Follow format from latest release in docs/releases/
→ Include issue numbers (#123) for all features/fixes
→ Keep under 120 lines
```

**Scenario 2: Version with Label-based Tracking**
```
User: "Generate release notes for v0.6.0"
→ Search for label:v0.6.0 closed issues
→ Search for merged PRs with v0.6.0 label
→ Extract implementation details from PR descriptions
→ Group by functional area (GeoTIFF, CDF, build system, etc.)
→ Generate release notes with proper attribution
```

**Scenario 3: Version from Project Board**
```
User: "Create release notes from project 'Version 1.0 Release'"
→ Query GitHub project board items
→ Filter for completed/closed items
→ Extract linked issues and PRs
→ Generate comprehensive release notes
→ Credit all contributors
```

## Notes
- If no milestone/label/project exists for the version, ask user to clarify which issues/PRs to include
- If some issues are still open, note them in "Known Limitations" section
- Adapt section order based on version content (e.g., skip "Dependencies" if none added)
- Use past tense for completed work, present tense for capabilities added
- Always verify issue/PR numbers are valid before including in release notes
- For versions with many issues (>20), prioritize most impactful features in Highlights
- If PR descriptions are sparse, read the actual code changes to understand implementation details