# Getting nep into conda-forge

The source of truth for the recipe is `conda/recipe/recipe.yaml` and
`conda/recipe/build.sh` in this repository; it builds the checkout
directly. `conda/make_staged_recipe.py` generates the staged-recipes
copy (release tarball source and version), so the two cannot drift.
The checkout recipe carries the next development version
(`4.2.1.dev0`); the script substitutes the release version.
`.github/workflows/conda.yml` builds and tests the recipe and lints
the generated staged copy on every push and PR.

## Prerequisites

- `captainkirk99` is the only recipe maintainer.
- `rattler-build` and `conda-smithy` in the conda env
  `/fast/ed/tmp-conda/env` (created by the first command in section 1;
  neither is packaged for apt). They are not on `PATH`, so call them as
  `/fast/ed/tmp-conda/env/bin/rattler-build` and
  `/fast/ed/tmp-conda/env/bin/conda-smithy`.
- The fork `captainkirk99/staged-recipes`, cloned at
  `/fast/ed/staged-recipes` with an `upstream` remote pointing at
  `conda-forge/staged-recipes`. Always rebase on `upstream/main`;
  the fork's own `main` is thousands of commits behind.
- A green `conda` CI run on NEP `main`; note its URL for the PR.

## 1. Build and lint locally

```sh
conda create -y -p /fast/ed/tmp-conda/env -c conda-forge rattler-build conda-smithy
curl -sSfL -o /fast/ed/tmp-conda/conda_build_config.yaml \
  https://raw.githubusercontent.com/conda-forge/conda-forge-pinning-feedstock/main/recipe/conda_build_config.yaml
/fast/ed/tmp-conda/env/bin/rattler-build build --recipe conda/recipe/recipe.yaml \
  -m /fast/ed/tmp-conda/conda_build_config.yaml \
  --output-dir /fast/ed/tmp-conda/out
```

A clean result: the build log's ctest summary is 100% passed
(including `tst_pdb_udf`, `tst_mmcif_udf`, `tst_fits_udf`,
`tst_pds4_udf`, `ftst_pdb_udf`, `ftst_mmcif_udf` and `ftst_fits_udf`),
every package test passes including the four `ncdump -h` lines (PDB, mmCIF, FITS, PDS4), and
the `.conda` contains no `.a` files and no `libncgeotiff`.

## 2. Generate the staged recipe

```sh
python3 conda/make_staged_recipe.py 4.2.0 \
  18656ce4f7095f3e52234645224144d2558ff001a7444fcc9b6ed7b7e718eccd \
  /fast/ed/tmp-conda/staged
/fast/ed/tmp-conda/env/bin/rattler-build build \
  --recipe /fast/ed/tmp-conda/staged/recipes/nep/recipe.yaml \
  -m /fast/ed/tmp-conda/conda_build_config.yaml \
  --output-dir /fast/ed/tmp-conda/out
/fast/ed/tmp-conda/env/bin/conda-smithy recipe-lint --conda-forge /fast/ed/tmp-conda/staged/recipes/nep
```

The build must pass against the 4.2.0 tarball and `recipe-lint` must
report no errors. Confirm the tarball sha256 before using it:
`curl -sL https://github.com/Intelligent-Data-Design-Inc/NEP/archive/refs/tags/v4.2.0.tar.gz | sha256sum`.

## 3. Update staged-recipes PR #34309 in place

PR #34309 ("Add nep 3.0.0") was opened 2026-07-26 from
`captainkirk99:nep-3.0.0`. Update it in place so it submits the
format-only 4.2.0 package instead of the old LZ4 build:

```sh
git -C /fast/ed/staged-recipes fetch origin nep-3.0.0
git -C /fast/ed/staged-recipes fetch upstream
git -C /fast/ed/staged-recipes switch -c nep-3.0.0 --track origin/nep-3.0.0
git -C /fast/ed/staged-recipes rebase upstream/main
git -C /fast/ed/staged-recipes rm recipes/nep/meta.yaml
cp /fast/ed/tmp-conda/staged/recipes/nep/{recipe.yaml,build.sh} \
  /fast/ed/staged-recipes/recipes/nep/
git -C /fast/ed/staged-recipes add recipes/nep
git -C /fast/ed/staged-recipes commit -m "nep: package format-only 4.2.0 (v1 recipe)"
git -C /fast/ed/staged-recipes push --force-with-lease origin nep-3.0.0
```

The force-push is needed because of the rebase. Ed runs it himself or
approves it explicitly. The branch name stays `nep-3.0.0`, because
GitHub cannot change a PR's head branch.

## 4. Edit the PR

- Retitle it `Add nep`.
- Body: the template checklist, all ticked; state that the package
  contains the PDB, mmCIF, FITS and PDS4 readers; why GeoTIFF (needs
  `4d920c5`, post-4.2.0), DICOM (conda-forge `gdcm` is built per
  Python version), GRIB2 (no `g2c` package), CDF (no NASA `cdf`
  package) and NEXTCDF-4 (needs HDF5 2.1.1; conda-forge pins 1.14.6)
  are off; that LZ4 moved to `nep-compress` (staged-recipes#35426);
  and a link to a green `conda` run.
- Comment: "Updated to the format-only 4.2.0 release with a v1
  recipe; ready for review." Ed's maintainer confirmation from
  2026-07-26 stays valid. For review, comment
  `@conda-forge-admin, please ping team` (first-time contributors
  cannot ping teams directly).

## 5. Review and feedstock creation

- When the staged-recipes checks are green (`linter`,
  `staged-recipes (build linux_64)`; the osx build reports skipped),
  ask for review as above. If review stalls, ask on the conda-forge
  Zulip.
- After merge, `conda-forge/nep-feedstock` is created with
  `captainkirk99` as maintainer.
- From then on, recipe changes go to the feedstock first and are
  mirrored back to `conda/recipe/` so the checkout recipe stays in
  sync.
- Remove the `Lint the staged-recipes copy` step and the micromamba
  step from `.github/workflows/conda.yml` once the feedstock exists.
  Keep `Fetch conda-forge pinnings`, because the build step uses it.

## 6. New releases

For the first release that contains `4d920c5`, turn on GeoTIFF: add
`geotiff` and `libtiff` to the host requirements, set
`-DNEP_ENABLE_GEOTIFF=ON`, and add a `libncgeotiff` test plus a `.tif`
`ncdump` test. Otherwise the autotick bot's version PR needs no
recipe changes unless a dependency or CMake option changed.

## 7. Adding macOS

macOS needs `.ncrc` templates that use
`${CMAKE_SHARED_LIBRARY_SUFFIX}` instead of hard-coded `.so` names.
That is not planned; the recipe stays `skip: not linux`. Windows is
not planned.

## Status

| Step | Date | Link |
| --- | --- | --- |
| Recipe in `conda/recipe/` | 2026-11-05 | `conda/recipe/` |
| First green `conda` CI run on `main` | pending | |
| Local build of the checkout recipe (section 1) | 2026-11-05 | ctest pass, package tests pass; `libnetcdf 4.10.1`, `hdf5 1.14.6` |
| Staged 4.2.0 recipe built and linted (section 2) | 2026-11-05 | package tests pass, `recipe-lint` clean |
| staged-recipes PR #34309 updated | pending | https://github.com/conda-forge/staged-recipes/pull/34309 |
| staged-recipes PR merged | pending | |
| Feedstock created | pending | |
| 4.2.0 published on conda-forge | pending | |

Author: Edward Hartnett, Intelligent Data Design, Inc.
