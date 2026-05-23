
l done. Here's a summary of every change:

New files:

File	Purpose
src/vcglib/vcg/.../collapse_event.h
CollapseEvent struct + gOnCollapse callback + gCollapseIdx counter
src/common/collapse_logger.h
JSONL writer — open(), write(event), close()
Modified files:

File	Change
tri_edge_collapse_quadric.h
#include collapse_event.h; Execute() collects deleted/modified faces and fires gOnCollapse before the topology changes
quadric_simp.h
meshName = "" optional param on QuadricSimplification
quadric_simp.cpp
Opens CollapseLogger, sets gOnCollapse lambda, clears it in Finalize
meshfilter.cpp
Passes m.label().toStdString() so the file is named after the mesh
Output (every time you run the filter on a mesh named e.g. bunny):

collapse_records/bunny_collapse_records.jsonl
Each line is one collapse, in order, with the edge, cost, deleted faces, and modified faces (pre-collapse coordinates).