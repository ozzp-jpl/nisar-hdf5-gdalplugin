# Upstreaming the NISAR driver to GDAL: remaining human items

`frmts/nisar/` is laid out as a GDAL in-tree driver. A local GDAL branch (`nisar-driver`,
based on OSGeo/gdal `master`, 3.14.0dev) has been prepared and validated:

- `frmts/nisar/` = the sources from this repository, with `CMakeLists.gdal.txt` copied to `CMakeLists.txt`
- registration: `gdal_dependent_format(nisar ... "GDAL_USE_HDF5")` in `frmts/CMakeLists.txt`,
  `NISAR` in `frmts/drivers.ini`, `GDALRegister_NISAR()` in `gcore/gdal_frmts.h` and
  `frmts/gdalallregister.cpp` (registered before HDF5)
- `doc/source/drivers/raster/nisar.rst` + entry in `doc/source/drivers/raster/index.rst`. Follows
  GDAL's "Building documentation" guidance: `.. _raster.nisar:` label, `shortname` /
  `versionadded` / `build_dependencies` / `supports_*` directives, `.. oo::` / `.. config::`
  definitions with `:oo:` / `:config:` references, `:program:`, `code-block:: console|bash`,
  anonymous `__` links. Verified with `cmake --build . --target html` (BUILD_DOCS=ON, Sphinx +
  Doxygen) - no warnings from the NISAR page - and `ctest -R doc-spelling` after adding
  `dataspace(s)` and `Kerchunk` to `doc/source/spelling_wordlist.txt`
- `autotest/gdrivers/nisar.py` + two static synthetic fixtures in `autotest/gdrivers/data/nisar/`
  (`NISAR_L2_GCOV_synthetic.h5`, 29 KB, and `NISAR_L1_RSLC_synthetic.h5`, 33 KB, written by
  `generate_synthetic.py` in the same directory; no h5py/numpy needed at test time).
  Follows GDAL's automated-testing guidance: `pytestmark = require_driver("NISAR")`,
  `gdaltest.GDALTest` checksum/geotransform/SRS test, `@pytest.mark.parametrize`, independent
  test cases, file-handle leak fixture (as in `hdf5.py`), negative Identify/Open cases
- verified: `-DGDAL_ENABLE_DRIVER_NISAR=ON` (builtin) and `-DGDAL_ENABLE_DRIVER_NISAR_PLUGIN=ON`
  (`gdalplugins/gdal_NISAR.so`) both build, and `autotest/gdrivers/nisar.py` passes in both modes
  and against the standalone plugin on GDAL 3.12

None of this has been posted, pushed or proposed upstream. Unchecked items below need a person,
except in section 6 where the remaining technical items are listed.

## 1. GDAL AI tool policy (blocking)

GDAL's [AI tool policy](https://github.com/OSGeo/gdal/blob/master/doc/source/community/ai_tool_policy.rst)
(linked from its `AGENTS.md`) bans "vibe-coded" contributions and agent-submitted PRs. LLMs may
only be used for autocomplete or repeated mechanical refactoring, any use must be disclosed, and
a human must be the primary author, responsible for every line.

- [ ] Decide how this policy applies to the driver itself and to the integration work prepared
      by Devin (registration, docs, autotest, the GetMetadata/CMake fixes, and the section 6
      driver changes: VSI `Identify()`, HDF5 global lock, `NISAR_EXPORT_ZARR_DIR`, connection-string
      parsing, `nisar_version.h`). Most likely the
      upstream patch has to be rewritten or line-by-line reviewed and owned by a JPL developer.
- [ ] Disclose AI assistance in the PR, as the policy requires.

## 2. JPL legal sign-off on licensing

- [x] License header choice. Approval to replace the former "ALL RIGHTS RESERVED" /
      export-control / commercial-negotiation notice was granted (relayed by O. Pariser,
      2026-11-16). All NISAR-authored C++ sources carry `Copyright 2025, California Institute of
      Technology. All rights reserved. U.S. Government sponsorship acknowledged.` followed by the
      Apache-2.0 notice and `SPDX-License-Identifier: Apache-2.0`; the Python tests and the
      README license section use the same wording, and the README export-control paragraph is
      gone. Keep the written approval on file; it is not stored in this repository.
- [ ] Note on the `frmts/mrf` precedent: in GDAL, the Caltech notice of `frmts/mrf` is a
      BSD-style (3-clause) license; the Apache-2.0 notice in that driver is Esri's. So GDAL
      already ships Apache-2.0 driver code and a Caltech copyright block, but not a
      Caltech + Apache-2.0 block. Word the request to the PSC accordingly.
- [ ] On upstream acceptance, add the Caltech block to GDAL's `LICENSE.TXT` (as for
      `frmts/mrf/*`).
- [ ] `hdf5vfl.{h,cpp}` derive from GDAL's MIT-licensed `frmts/hdf5/hdf5vfl.h`. They keep the
      original MIT notice (Denis Nadeau, Sam Gillingham, Even Rouault) and `SPDX-License-Identifier: MIT`,
      with a Caltech line for the modifications. Confirm this is acceptable.
- [ ] GDAL core code is MIT. Ask whether GDAL accepts an Apache-2.0 driver in-tree (some code in
      GDAL is Apache-2.0, see `LICENSE.TXT`), or whether the upstream copy must be relicensed to
      MIT. Asked upstream; answer pending. A relicense to MIT needs a new Caltech approval.
- [ ] `autotest/gdrivers/nisar.py` and `nisar.rst` were written with a Caltech/MIT header, matching
      GDAL autotest convention. Confirm.

## 3. gdal-dev RFC / proposal

- [x] Draft of the post: [`docs/GDAL_DEV_RFC_DRAFT.md`](docs/GDAL_DEV_RFC_DRAFT.md) (scope,
      dependencies, maintenance commitment, test data plan, licensing, standalone driver vs.
      HDF5 subdriver, AI-tool disclosure).
- [ ] A JPL developer rewrites/owns the draft, fills in the `<...>` placeholders (maintainer
      names, duration of the commitment, minimum HDF5 version, test-data sizes and
      redistribution rights) and posts it to
      [gdal-dev](https://lists.osgeo.org/mailman/listinfo/gdal-dev) under their own name.
- [ ] Decide `frmts/nisar` (separate driver, as prepared) vs. an HDF5 subdriver
      (`frmts/hdf5/nisardataset.cpp`, like BAG / S-102 / S-104 / S-111). Arguments:
  - separate driver: own `NISAR:` prefix, own open options, independent plugin, reuses
    nothing from `HDF5Dataset` today;
  - HDF5 subdriver: matches how GDAL handles other HDF5 profiles, shares the HDF5 global
    lock, the `hdf5drivercore` deferred-plugin machinery and `hdf5vfl.h` (the copied VFL
    could be dropped), one HDF5 plugin to package.

## 4. Test data hosting

- [x] GDAL CI does not install h5py, so the autotest ships static fixtures
      (`autotest/gdrivers/data/nisar/*.h5`, regenerated with `generate_synthetic.py`) instead
      of generating them at run time.
- [x] L1 (RSLC swaths, GCPs), complex and derived-subdataset, mask, metadata-cube
      (`QUANTITY`/`DEM_FILE`, L1 and L2), `/vsimem/`, `/vsizip/`, `/vsicurl/` (local test
      server), Kerchunk-export and multi-threaded-read coverage on the synthetic fixtures:
      47 tests pass in-tree (GDAL 3.14.0dev, builtin driver).
- [x] Real-granule tests (RSLC, GSLC, GCOV, GUNW: subdatasets, L1 GCPs, derived subdatasets,
      masks, interpolation, `/vsicurl/`) are written and marked
      `@pytest.mark.require_run_on_demand` / `slow`. **They have never been run**: they
      download from `OSGeo/gdal-test-datasets/nisar/`, which does not exist yet.
- [ ] Produce spatial subsets of real granules (the full ones are 40 MB to 1 GB), confirm
      redistribution rights, propose them to
      [OSGeo/gdal-test-datasets](https://github.com/OSGeo/gdal-test-datasets), then set the
      file names in `REAL_GRANULES` in `nisar.py` and run the tests with `RUN_ON_DEMAND=YES`.

## 5. CLA / commit authorship

- [ ] Confirm what GDAL requires from contributors (CLA, DCO or nothing) and whether Caltech
      needs to sign anything on behalf of JPL employees.
- [ ] Pick the commit author(s) (a JPL developer, real name and `@jpl.nasa.gov` address) and
      decide how to credit the original driver authors.

## 6. Technical items found during the in-tree build

These are not blocking for the standalone plugin, but GDAL reviewers will likely ask for them:

- [x] Compiler warnings under GDAL's flags (`-Weffc++`, `-Wold-style-cast`, `-Wreorder`,
      `-Wshadow`, unused functions/variables): the driver now builds warning-free with
      `add_gdal_driver()` on GCC; re-check with clang and MSVC in GDAL CI.
- [x] GDAL development-practice pass (`doc/source/development/dev_practices.rst`): x*y
      allocations use `VSI_MALLOC2_VERBOSE`, large `std::vector` sizes are guarded against
      `std::bad_alloc`, classes with pointer members use `CPL_DISALLOW_COPY_ASSIGN`, no
      old-style casts, lower-case `.cpp`/`.h` filenames, `.clang-format` applied.
- [x] `#pragma message` lines and commented-out debug pragmas removed from `nisardataset.h`.
- [ ] Deferred plugin loading: GDAL's HDF5 driver splits `Identify()` into
      `hdf5drivercore.cpp` (`CORE_SOURCES`, `NO_SHARED_SYMBOL_WITH_CORE`) so a plugin is only
      loaded when needed. `NisarDataset::Identify()` calls `H5Fopen`, so it cannot be moved into
      core as is.
- [x] `Identify()` opens local files and non-network virtual paths (`/vsimem/`, `/vsizip/`,
      ...) through the VSI virtual file driver and checks for
      `/science/{LSAR,SSAR}/identification`.
- [ ] `Identify()` still claims network files (`/vsicurl/`, `/vsis3/`, `http(s)://`, `s3://`)
      from their name (it must contain `NISAR`) instead of opening them, to avoid range
      requests on every remote HDF5 file GDAL probes. Reviewers may ask for a content check;
      the `NISAR:` prefix always works.
- [x] Thread safety: libhdf5 calls are serialised by a global recursive lock
      (`NISAR_HDF5_GLOBAL_LOCK()`), taken when `H5is_library_threadsafe()` reports a
      non-thread-safe library, or forced with `-DGDAL_ENABLE_HDF5_GLOBAL_LOCK=ON|OFF` as in
      `frmts/hdf5`. Chunk decompression threads do not call libhdf5. The lock is separate from
      the HDF5 driver's: a process using both drivers from different threads on a
      non-thread-safe libhdf5 is still unprotected (sharing the lock is one argument for the
      HDF5-subdriver option). The autotest passes with the lock forced on and with the run-time
      default (lock off: the libhdf5 of the test environment is thread-safe).
- [x] `NISAR_EXPORT_ZARR` no longer writes to `/tmp`: the Kerchunk sidecar goes to the directory
      given by the new `NISAR_EXPORT_ZARR_DIR` config option, and nothing is written without it.
- [ ] The `#if GDAL_VERSION_*` compatibility blocks (GDAL < 3.12 / < 3.13) are not needed
      in-tree; keep them only in the standalone copy, or drop them once the plugin requires a
      recent GDAL.
- [x] `DRIVER_VERSION` comes from `frmts/nisar/nisar_version.h` and is reported by in-tree
      builds too (`v0.7.2`; the standalone plugin appends its build date).
- [x] `NISAR:` connection strings whose file name contains a colon (`http://host:8080/x.h5`,
      `C:/x.h5`) are parsed correctly; band reads and the Kerchunk export use the VSI path the
      file was opened with.
- [ ] Not built with MSVC or clang in-tree, and not run in GDAL's CI.
- [ ] `versionadded:: 3.14` in `nisar.rst` is a guess; set it to the release the driver
      lands in.

## 7. Behaviour change to release-note

- [ ] The driver no longer sets `GDAL_HTTP_MAX_RETRY=5` when it is unset (a driver must not
      change global configuration). Users reading remote granules should set it themselves.
      Add this to the next release's notes and the HOWTO version history.
- [ ] `NISAR_EXPORT_ZARR=YES` alone no longer writes `/tmp/nisar_kerchunk_*.json`; set
      `NISAR_EXPORT_ZARR_DIR`. Listed in the HOWTO "unreleased" row; add to the release notes.
