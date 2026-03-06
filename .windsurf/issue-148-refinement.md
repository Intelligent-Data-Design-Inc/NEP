## Issue Refinement Summary — #148

### Executive Summary
`quickstart.c` and `f_quickstart.f90` read global and variable attributes but never
compare their values against expected strings, so attribute corruption goes undetected.
Both files also accumulate data-check errors instead of failing fast on the first
mismatch. The fix is purely additive string comparisons plus a one-line restructure of
the data loop in each file.

### Requirements & Acceptance Criteria
- [ ] `quickstart.c`: `description` attribute compared to `"a quickstart example"`; print error + `return 1` on mismatch
- [ ] `quickstart.c`: `units` attribute compared to `"m/s"`; print error + `return 1` on mismatch
- [ ] `quickstart.c`: data loop exits immediately on first element mismatch (no error-count accumulation)
- [ ] `f_quickstart.f90`: same three changes using Fortran idioms (`trim()` comparison, `stop 1`)
- [ ] All existing tests pass after changes

### Implementation Plan
Minimal surgical edits to two files only:

**`examples/classic/quickstart.c`**
- After `nc_get_att_text(..., "description", desc_in)`, add `strcmp` check → print + `return 1`
- After `nc_get_att_text(..., "units", units_in)`, add `strcmp` check → print + `return 1`
- Replace error-counting loop with immediate `return 1` on first mismatch; remove `errors` variable

**`examples/f_classic/f_quickstart.f90`**
- After `nf90_get_att(..., "description", desc_in)`, add `trim(desc_in) /= "a quickstart example"` check → print + `stop 1`
- After `nf90_get_att(..., "units", units_in)`, add `trim(units_in) /= "m/s"` check → print + `stop 1`
- Replace error-counting loop with immediate `stop 1` on first mismatch; remove `errors` variable

### Next Steps
Use `/implement 148` to execute the code changes and run the test suite.
