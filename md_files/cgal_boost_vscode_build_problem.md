It didn’t happen before because configure was taking a consistent path, and after you came back it started taking a mixed path that CGAL 5.6 can’t handle.

What worked before
On earlier successful configures, CMake logged something like:

CGAL - using downloaded source (headers in src/external/downloads/CGAL-5.6/)
Boost - using downloaded source (headers in src/external/downloads/boost_1_87_0/)
Those two were built to work together. MeshLab’s download path even ships CGAL’s Windows GMP/MPFR under CGAL-5.6/auxiliary/gmp/, so configure didn’t need vcpkg’s CGAL/GMP find logic.

Nothing “broke” in the source — the dependency resolver started choosing different libraries.

What changed after you closed Cursor
Several things stacked:

1. Fresh configure on open
settings.json has "cmake.configureOnOpen": true. Closing and reopening Cursor often triggers a full re-configure, not just an incremental build. That re-runs all the find_package / download logic with today’s vcpkg state.

2. vcpkg had more packages than before
Your vcpkg tree already had things like CGAL and Boost. On a new configure:

find_package(Boost) succeeded → vcpkg Boost (newer MPL)
We briefly also hit vcpkg CGAL → GMP/MPFR find failures (separate issue)
So you could get downloaded CGAL headers + vcpkg Boost headers in the same compile — that’s the explosion in CGAL/number_utils.h (boost::mpl::if_c, etc.).

3. Our GMP “fix” made it worse for a while
To get past configure errors, we set MESHLAB_ALLOW_SYSTEM_CGAL=OFF so configure used bundled CGAL again — but Boost was still coming from vcpkg because that option defaults to ON. That’s exactly the bad combo you saw in the build log.

4. Incremental build hid the exe timestamp issue
Even when only plugins failed, meshlab.exe might not rebuild (unchanged sources), so it looked like “nothing updated” — that’s unrelated to CGAL, but it added confusion.

Timeline in one picture
Before (stable):
  bundled CGAL 5.6  +  bundled Boost 1.87  →  OK
After reopen (unstable):
  bundled CGAL 5.6  +  vcpkg Boost (new)  →  hundreds of CGAL header errors
After preset fix:
  bundled CGAL 5.6  +  bundled Boost 1.87  →  OK again
Why it felt tied to “closing Cursor”
You didn’t change MeshLab code for CGAL/Boost. Reopening triggered configure again, and vcpkg’s installed packages steered Boost (and sometimes CGAL) onto paths that weren’t used in the earlier session. The old session likely still had a good CMake cache from when both were bundled, so Build alone kept working until cache was cleared or presets changed.

What we locked in now
In CMakePresets.json:

MESHLAB_ALLOW_SYSTEM_CGAL=OFF
MESHLAB_ALLOW_SYSTEM_BOOST=OFF
So every configure uses the same bundled pair unless you change those flags on purpose.

Short answer: It didn’t happen before because you were consistently on bundled CGAL + bundled Boost. It happened after reopen because configure mixed old CGAL headers with new vcpkg Boost — and we’ve pinned both back to bundled in the preset.