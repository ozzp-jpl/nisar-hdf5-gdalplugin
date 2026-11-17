# Draft gdal-dev post: proposed NISAR raster driver (`frmts/nisar`)

> **Draft for human review, not posted anywhere.** This text was drafted with an AI coding
> agent (Devin). GDAL's [LLM tool policy](https://gdal.org/en/latest/community/ai_tool_policy.html)
> requires a human primary author who understands and owns every line, and bans agent-submitted
> contributions, mailing-list posts included. A JPL developer has to rewrite or own this text,
> fill in the `<...>` placeholders and post it under their own name.
> Open human items are tracked in [`UPSTREAM_CHECKLIST.md`](../UPSTREAM_CHECKLIST.md).

**To:** gdal-dev@lists.osgeo.org
**Subject:** Proposal: read-only NISAR (NASA-ISRO SAR) HDF5 raster driver

Hi all,

JPL maintains a GDAL raster driver for the HDF5 products of the NASA-ISRO SAR (NISAR)
mission. It is distributed today as an out-of-tree plugin
(<https://github.com/ozzp-jpl/nisar-hdf5-gdalplugin>, conda package `gdal-driver-nisar`).
We would like to contribute it to GDAL and are asking, before opening a pull request,
whether the PSC wants it, in which form, and whether an RFC is needed (RFC 85 applies).

## Scope

- Read-only raster driver, short name `NISAR`, connection string
  `NISAR:"<file>":<hdf5 path>` (the generic HDF5 driver still opens the same files).
- Products: L1 `RSLC`, `RIFG`, `RUNW` (radar coordinates, GCPs from
  `metadata/geolocationGrid`); L2 `GSLC`, `GCOV`, `GUNW`, `GOFF` and L3 `SME2` (geotransform
  from the coordinate vectors, SRS from the `projection` dataset).
- Subdatasets for every raster layer, selection by open options (`INST`, `FREQ`, `POL`),
  per-product validity masks (`MASK=YES`), HDF5 attributes as metadata domains.
- Interpolation of the 3-D radar-grid metadata cubes (incidence angle, slant range, ...)
  onto the product grid with a user-supplied DEM (`QUANTITY` + `DEM_FILE`).
- Cloud reads: GDAL block size = HDF5 chunk size; the driver gets the chunk map from libhdf5
  once, then reads neighbouring chunks through VSI in one range request and decompresses
  them in threads. libhdf5 I/O goes through a VSI-backed virtual file driver (the same
  approach as `frmts/hdf5/hdf5vfl.h`), so `/vsis3/`, `/vsicurl/`, `/vsimem/`, ... work.
- Not in scope: creation or update, the multidimensional API, vector data.

Size: about 10 000 lines of C++17 in 15 files, plus the documentation page and
`autotest/gdrivers/nisar.py`.

## Dependencies

- libhdf5 (`GDAL_USE_HDF5`) with `H5Dchunk_iter()`: HDF5 1.14 or later according to the HDF5
  headers. Tested with HDF5 2.x only; `<minimum version to confirm>`.
- zlib (GDAL's; the standalone plugin can use zlib-ng). Nothing else, no binary SDK.
- Builds with `add_gdal_driver()` as builtin or plugin
  (`gdal_dependent_format(nisar ... "GDAL_USE_HDF5")`). The in-tree build has been done with
  GCC on Linux x86_64 only; the standalone plugin is also built on Linux aarch64 and macOS
  arm64. **MSVC and GDAL's CI configurations have not been tried.**

## Standalone driver or HDF5 subdriver?

This is the main question for the list. The prepared branch adds `frmts/nisar` as a separate
driver. The alternative is a subdriver in `frmts/hdf5`, like BAG, S-102, S-104 and S-111.

| | `frmts/nisar` (as prepared) | subdriver in `frmts/hdf5` |
| --- | --- | --- |
| Code reuse | Reuses nothing from `HDF5Dataset`. Carries a copy of the VFL (`hdf5vfl.{h,cpp}`, MIT notice kept) and its own HDF5 global lock | Shares `hdf5vfl.h`, `HDF5_GLOBAL_LOCK()` and the build-time thread-safety detection; the copies go away |
| Identification | `Identify()` calls libhdf5, so it cannot go into a deferred-loading `*drivercore.cpp` as is. Network files are claimed from their name (must contain `NISAR`) to avoid extra requests | Could be done in `hdf5drivercore.cpp` from the header bytes, as for BAG / S-10x |
| Packaging | One more plugin, can be disabled on its own | One HDF5 plugin |
| Reading | Does not use `H5Dread` for pixels (own chunk reader over VSI) | Same code, would sit beside `HDF5ImageDataset` |
| Effort | Ready for review | Needs a port to the `frmts/hdf5` conventions |

We have no strong preference and will follow the PSC's choice.

## Maintenance commitment

- Responsible contact (RFC 85, "Maintainers per sub-system"): `<name, @jpl.nasa.gov, GitHub id>`.
- Backup contact: `<name>`.
- Commitment: `<e.g. for the duration of the NISAR mission operations and data reprocessing>`.
  JPL would triage NISAR tickets, follow GDAL API/CI changes and keep the driver in step with
  NISAR product-specification revisions.
- The out-of-tree plugin would `<be retired / be kept for older GDAL versions>` once the driver
  is in a GDAL release.

## Test data plan

- In-tree (`autotest/gdrivers/data/nisar/`): two synthetic granules written by a small
  h5py script kept next to them, 29 KB (L2 GCOV-like: 2 Float32 layers, mask, incidence-angle
  cube) and 33 KB (L1 RSLC-like: 2 CFloat32 layers, geolocation grid). They cover
  identification, subdatasets, geotransform/SRS, GCPs, complex and derived subdatasets,
  masks, cube interpolation against an in-memory DEM, `/vsimem/`, `/vsizip/`, `/vsicurl/`
  (local test server) and multi-threaded reads. No network, h5py or numpy at test time.
- Real products are 40 MB to 1 GB. We propose spatial subsets of one RSLC, GSLC, GCOV and
  GUNW granule in OSGeo/gdal-test-datasets, used by tests marked `require_run_on_demand` and
  `slow`. These tests are written but **the subsets do not exist yet**; sizes: `<to fill>`.
  NISAR data are openly distributed by NASA (ASF DAAC); redistribution of subsets is
  `<to be confirmed by JPL>`.

## Licensing

- The driver sources are Copyright California Institute of Technology, U.S. Government
  sponsorship acknowledged, under **Apache-2.0** (`SPDX-License-Identifier: Apache-2.0`).
- `hdf5vfl.{h,cpp}` derive from GDAL's `frmts/hdf5/hdf5vfl.h` and keep its MIT notice.
- The autotest script and the documentation page carry a Caltech copyright with
  `SPDX-License-Identifier: MIT`.
- Question for the PSC: is an Apache-2.0 driver acceptable in-tree? Apache-2.0 code already
  ships in GDAL (for example the Esri-copyright parts of `frmts/mrf`, listed in `LICENSE.TXT`
  next to the Caltech BSD-style notice of that driver). If accepted, we would add the Caltech
  notice to `LICENSE.TXT`. If MIT is required for the in-tree copy, we will ask Caltech for
  that relicensing.

## Use of AI tools (disclosure)

`<To be written by the submitting developer.>` Facts to disclose: the driver was written by
JPL developers; an AI coding agent was then used for the in-tree integration (registration,
CMake, documentation page, autotest), for warning and coding-practice clean-ups, and for
some driver changes (VSI identification, HDF5 global lock, config options). State which
parts were reviewed or rewritten by a human, and who takes responsibility for them. If that
cannot be brought within the policy, say so and ask how the PSC wants to proceed.

## Questions

1. Is the driver of interest for GDAL, and is an RFC required or is a pull request enough?
2. `frmts/nisar` or a subdriver of `frmts/hdf5`?
3. Apache-2.0 in-tree, or MIT?
4. Is OSGeo/gdal-test-datasets the right place for the real-granule subsets, and what is
   an acceptable size?

Thanks,
`<name>`, Jet Propulsion Laboratory, California Institute of Technology
