at's expected — it's incremental build behavior working correctly.

meshlab.exe is built from src/meshlab/ (the main app shell). The changes you just made were all in:

filter_meshing.dll ← quadric_simp.cpp, meshfilter.cpp
collapse_event.h / collapse_logger.h ← new headers
MSBuild checks each target's dependencies. Since none of the source files that compile into meshlab.exe were touched, it skipped relinking it entirely and left the old binary untouched. That's why the timestamp is from this morning.

filter_meshing.dll on the other hand should have a fresh timestamp from your last build. That's the file that actually changed:

Exactly as expected:

File	Timestamp	Reason
meshlab.exe
6:45 AM
Not rebuilt — its sources were unchanged
filter_meshing.dll
1:05 PM
Rebuilt — contains your quadric_simp.cpp changes
So the old meshlab.exe timestamp is not a problem at all. The logging code lives in filter_meshing.dll, which is fresh. When meshlab.exe runs and loads that DLL at startup, your new collapse logger will be active.