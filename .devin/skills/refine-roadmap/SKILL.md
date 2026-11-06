---
name: refine-roadmap
description: Turn one sprint of docs/roadmap.md into a researched, decision-complete plan in docs/plans/ and tighten the roadmap entry
argument-hint: "<version> sprint <N>   e.g. v1.1.0 sprint 2"
triggers:
  - user
allowed-tools:
  - read
  - grep
  - glob
---

Refine the sprint named in the arguments (for example `v1.1.0 sprint 2`).
If the version or sprint is missing or does not match a heading in
`docs/roadmap.md`, list the versions and sprints that exist and stop.

This skill plans. It does not implement anything.

## 1. Research

Do this before asking any questions, so the questions cover real
decisions and not facts you could have looked up.

- Read `docs/roadmap.md`. Note the sprint's text, the other sprints in
  the same version, and any later versions that depend on it.
- Read every existing plan in `docs/plans/`, especially earlier sprints
  of the same version. Carry over their decisions, their "Out of scope"
  items aimed at this sprint, and any notes that hand work to this sprint.
- For each GitHub issue the sprint references, run
  `gh issue view <n> --repo captainkirk99/nep-compress --comments`.
  Record the title, the reported problem, and any proposed fix.
- Read the code, build files, tests, CI workflows, and docs the sprint
  will touch. Record what exists now, with file paths.
- If the sprint involves NetCDF or HDF5 filters, plugins, chunking, or
  quantization, invoke the `netcdf` skill and check every filter ID and
  API claim against it and the installed `netcdf_filter.h`.
- Check the tools and versions the work needs (compilers, doxygen, CMake,
  libraries) and whether `ubuntu-latest` CI provides them.
- Read the Spack recipe
  `spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py`
  (if it exists) and `docs/spack-plan.md`. Note which of the sprint's
  changes the recipe would have to follow: new or renamed CMake options
  or cache paths, new or changed dependencies and minimum versions, new
  plugins or installed files, install-layout changes, and test
  requirements such as plugin directories.

Mark anything you could not verify as an open question, not a fact.

## 2. Ask clarifying questions

Ask 3-5 questions, all in one message. Number them. Give each one 2-4
lettered multiple-choice answers and mark one as **(Recommended)** with a
one-line reason. Ed may reply with just the letters (for example
`1a 2b 3a`).

Ask only about decisions that change the plan: scope, interfaces and
names, behavior changes, compatibility, test strategy, CI, and release
impact. Do not ask about anything the research already settled.

Wait for the answers before writing anything.

## 3. Write the plan

Create `docs/plans/<version>-sprint<N>-<slug>.md`, where `<slug>` is a
short kebab-case form of the sprint title (for example
`v1.1.0-sprint1-documentation.md`). Use these sections, in this order:

1. `# <Version> Sprint <N>: <Title>`, then
   `Source: docs/roadmap.md, "<version heading> / Sprint <N>".`
2. **Goal**: one paragraph on the result, not the steps.
3. **Decisions**: a table of topic and decision, including Ed's answers.
4. **Current state (<today's date>)**: verified facts with file paths.
5. **Work items**: numbered subsections. Each names the files to change,
   the exact interfaces (function names, signatures, CMake options, file
   names), and the tricky cases. Leave no choices open for the
   implementer.
6. **Verification**: exact commands, the tests to add or change, and
   what has to pass locally and in CI.
7. **Out of scope**: what this sprint does not do, and which sprint or
   version handles it.

Project conventions the plan must follow:

- Author string: `Edward Hartnett, Intelligent Data Design, Inc.`
  (`@author` in Doxygen, plain `Author:` in test code). Test code under
  `test/` uses plain comments, never Doxygen markup.
- New public functions need full Doxygen blocks in the installed header.
- Every change keeps `ctest` passing and the docs build warning-free.
- If the sprint changes a name, signature, or behavior that is already
  documented, the plan includes updating those docs.
- If the sprint changes anything the Spack recipe depends on (CMake
  options, dependencies, minimum versions, installed files, plugin
  directories), the plan includes a work item that updates the overlay
  recipe. That item says whether the change needs a new variant, a
  `depends_on` change, or a version-gated `when=` condition, so older
  release versions in the recipe still build. Its Verification runs
  `spack audit packages nep-compress`, `spack spec`, and upstream's
  style checks (`ruff format --diff`, `ruff check --no-fix` and
  `vermin`, as in `docs/spack-plan.md` section 2), and expects the
  `spack` CI job to pass. If the change also has to
  reach `spack/spack-packages`, the plan says it ships in the next
  version-bump PR, following `docs/spack-plan.md`. If the sprint
  changes nothing the recipe depends on, the plan's Current state says
  so in one line.

## 4. Update the roadmap

In `docs/roadmap.md`, replace the sprint's text with a short, refined
summary of 3-8 lines: the scope as decided and the items now out of
scope. Then add a line `Plan: [docs/plans/<file>](plans/<file>)`. Do not
edit other sprints, except to add an item this sprint moved out of
scope to the sprint that now owns it.

## 5. Stop

Reply with the plan path, a 3-5 line summary, and any open questions
left. Do not implement, commit, or push.
