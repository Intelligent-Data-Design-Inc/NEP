---
description: Generate GitHub release notes from roadmap
auto_execution_mode: 3
---

Generate professional GitHub release notes for a specific version by extracting information from `docs/roadmap.md` and following the established release notes format.

## Prerequisites
1. Read `docs/roadmap.md` to identify available versions
2. Check `docs/releases/` for the latest release notes format examples (v0.4.0.md is comprehensive)
3. Identify which version to document (user will specify, e.g., "0.5", "1.0")

## Steps

### 1. Extract Version Information from Roadmap
Read `docs/roadmap.md` and locate the specified version section. Extract:
- **Version number and title** (e.g., "Version 0.5 - Spam Database Management")
- **One-line description** from the subtitle
- **Release Date** (if specified)
- **Features list** (all bullet points under Features section)
- **Technical Deliverables** (all bullet points under that section)
- **Sprints information** (if present, summarize key implementation details)

### 2. Review Existing Release Notes Format
Examine the most recent release notes (e.g., `docs/releases/v0.4.0.md`) to understand:
- Section structure and order
- Writing style (professional, concise, technical)
- How features are organized (Backend/Frontend or by category)
- Level of technical detail expected

### 3. Create Release Notes Structure
Generate a markdown file named `docs/releases/v{VERSION}.md` with these sections:

**Required Sections:**
- **Title & Subtitle**: Version number + main theme, followed by `>` blockquote with one-line description
- **Highlights**: 4-6 bullet points of major achievements (start with bold emphasis)
- **Features**: Organized by functional areas with detailed descriptions (UDF handlers, compression, build system, etc.)
- **API Changes**: New/modified NetCDF API extensions, NC_Dispatch implementations (if applicable)
- **Build System**: CMake and Autotools changes, new dependencies, configuration options
- **Testing**: Coverage metrics, test strategies, known test gaps
- **Documentation**: Doxygen updates, API documentation, user guides (if applicable)
- **Dependencies Added**: New libraries (HDF5, NetCDF-C, NCEPLIBS-g2, etc.) with versions
- **Known Limitations**: Issues deferred or not addressed (if applicable)
- **Breaking Changes**: None or list with migration guidance

**Footer Section:**
- **Released**: Date (format: YYYY-MM-DD, use TBD if unknown)
- **Scope**: List sprints or major work packages included
- **Team Notes**: One-sentence summary of readiness for next phase

### 4. Writing Guidelines
- **Be concise**: Keep total length under 100 lines (~1 page when rendered)
- **Use technical language**: Include file paths, function names, collection names
- **Emphasize user impact**: Start features with user-facing benefits
- **Cross-reference docs**: Link to architecture docs, API specs, testing guides
- **Maintain consistency**: Match tone and structure of previous releases
- **Prioritize information**: Most important features first, details second

### 5. Content Transformation Rules
When converting roadmap content to release notes:
- **Features → Highlights**: Extract 4-6 most impactful features for Highlights section
- **Features → Features**: Expand with implementation details (what was built, where in codebase, which source files)
- **Technical Deliverables → Multiple sections**: Split across Build System, Testing, Documentation, Dependencies
- **Sprint Tasks → Implementation details**: Summarize what was completed, not planned
- **Future work → Next Steps**: Convert deferred items into forward-looking statements

### 6. Quality Checks
Before finalizing:
- ✅ Version number matches roadmap (format: v0.X.0 or v1.X.0)
- ✅ File name matches version (e.g., `v0.5.0.md` for Version 0.5)
- ✅ All sections present (even if "None" or "TBD")
- ✅ Length is under 100-120 lines
- ✅ No placeholder text like "[TODO]" or "[INSERT]"
- ✅ Technical terms are accurate (verify collection names, endpoint paths)
- ✅ Consistent formatting (bold for emphasis, code blocks for file paths)

### 7. Finalization
- Save the file to `docs/releases/v{VERSION}.md`
- Do NOT commit or push (per user rules)
- Inform the user the release notes are ready for review

## Example Usage
User: "Create release notes for version 0.5"
→ Read roadmap section for Version 0.5
→ Generate `docs/releases/v0.5.0.md`
→ Follow format from v0.4.0.md
→ Keep under 100 lines
→ Include all required sections

## Notes
- If version doesn't exist in roadmap, ask user to clarify
- If version is incomplete (e.g., sprints not finished), note TBD items clearly
- Adapt section order based on version content (e.g., skip "Dependencies" if none added)
- Use past tense for completed work, present tense for capabilities added