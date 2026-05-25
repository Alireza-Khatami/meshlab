# Initial Heap State Schema

## Overview

Before any edge collapse runs, **Simplification: Quadric Edge Collapse Decimation** writes one snapshot of every candidate collapse in the priority heap:

```
collapse_records/<meshName>_initial_heap_state_.json
```

- Written **once**, immediately after `Init()`, **before** the first `DoOptimization()` step.
- One JSON document (not JSONL).
- `entries` are sorted by **ascending `cost`** (cheapest collapse first — same order the heap will process).
- Related per-collapse log: see `collapse_records_schema.md` (`<meshName>_collapse_records.jsonl`).

---

## File naming

| Pattern | Example |
|---------|---------|
| `collapse_records/<meshName>_initial_heap_state_.json` | `collapse_records/bunny_initial_heap_state_.json` |

`<meshName>` is the MeshLab layer label (`m.label()`), passed into `QuadricSimplification`.

---

## Document structure

```json
{
  "mesh": "<string>",
  "heap_size": <integer>,
  "order": "ascending_by_cost",
  "entries": [
    {
      "rank": <integer>,
      "v0_id": <integer>,
      "v1_id": <integer>,
      "cost": <float>
    }
  ]
}
```

---

## Field reference

### Root

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `mesh` | `string` | yes | Mesh label from MeshLab |
| `heap_size` | `integer` | yes | Number of candidate collapses (`entries.length`) |
| `order` | `string` | yes | Always `"ascending_by_cost"` |
| `entries` | `array` | yes | All heap candidates, sorted by `cost` ascending |

### `entries[]` (one per candidate edge collapse)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `rank` | `integer` | yes | 0-based index after sorting; `0` = lowest cost (collapsed first) |
| `v0_id` | `integer` | yes | Global vertex index of the vertex that **would be deleted** |
| `v1_id` | `integer` | yes | Global vertex index of the vertex that **would survive** (moved to optimal position on collapse) |
| `cost` | `float` | yes | QEM priority from `ComputePriority()` at heap build time |

### Edge semantics

- Each entry is a directed collapse **v0 → v1** (same as `TriEdgeCollapseQuadric`).
- `v0_id` and `v1_id` are indices into the mesh vertex array at simplification start (before any collapse in this run).
- The pair may appear only once in the heap (symmetric optimal placement) or twice (asymmetric collapses) depending on MeshLab / VCGLib settings.

---

## Cost precision

`cost` is formatted with **`%.17g`** (full IEEE-754 `double` string form), **not** 6 decimal places.

| Raw value | Bad (6 decimals) | Logged (`%.17g`) |
|-----------|------------------|------------------|
| `1.23e-12` | `0.000000` | `1.23e-12` |
| `4.56e-8` | `0.000000` | `4.56e-08` |
| `0.000042` | `0.000042` | `0.000042` |

Use a JSON parser that reads floating-point literals as `double` (Python `json`, etc.).

---

## Example (minimal)

```json
{
  "mesh": "bunny",
  "heap_size": 3,
  "order": "ascending_by_cost",
  "entries": [
    { "rank": 0, "v0_id": 42, "v1_id": 37, "cost": 1.23456789012345e-12 },
    { "rank": 1, "v0_id": 10, "v1_id": 11, "cost": 2.34567890123456e-11 },
    { "rank": 2, "v0_id": 5,  "v1_id": 8,  "cost": 0.000123456789012345 }
  ]
}
```

---

## Relationship to the collapse JSONL log

| File | When | Content |
|------|------|---------|
| `_initial_heap_state_.json` | Before collapse #0 | All candidates + costs only |
| `_collapse_records.jsonl` | After each collapse | One line per performed collapse (edge, cost, faces, positions) |

To replay simplification:

1. Start from the original mesh.
2. Optionally use `_initial_heap_state_.json` to verify which edge should be chosen next at each step.
3. Apply `_collapse_records.jsonl` lines in `idx` order for the actual sequence MeshLab executed.

---

## Parsing (Python example)

```python
import json

with open("collapse_records/bunny_initial_heap_state_.json") as f:
    heap = json.load(f)

assert heap["order"] == "ascending_by_cost"
assert len(heap["entries"]) == heap["heap_size"]

# Cheapest edge first
for e in heap["entries"]:
    print(e["rank"], e["v0_id"], e["v1_id"], e["cost"])

# Edge -> cost map (note: directed; v0,v1 != v1,v0)
cost_by_edge = {(e["v0_id"], e["v1_id"]): e["cost"] for e in heap["entries"]}
```

---

## Source files

| File | Role |
|------|------|
| `src/common/collapse_logger.h` | `writeInitialHeap()`, `fmtCost()` |
| `src/meshlabplugins/filter_meshing/quadric_simp.cpp` | Collects heap after `Init()`, calls `writeInitialHeap()` |
| `src/meshlabplugins/filter_meshing/quadric_simp.h` | `MyTriEdgeCollapse::edgeCost()` |
| `src/meshlabplugins/filter_meshing/meshfilter.cpp` | Passes `m.label()` as `meshName` |





atures are implemented. Here’s where files go and which source files are involved.

Where files are saved
Paths are relative to MeshLab’s current working directory (not the repo root):

collapse_records/<meshName>_initial_heap_state_.json
collapse_records/<meshName>_collapse_records.jsonl
<meshName> is the layer label in MeshLab (m.label()), e.g. bunny →
collapse_records/bunny_initial_heap_state_.json.

If you launch from VS Code (launch.json sets cwd to the Debug distrib folder), files usually end up here:

D:\projects\graphics\meshlab\build\src\distrib\Debug\collapse_records\
The folder collapse_records/ is created automatically if it doesn’t exist (_mkdir in the logger).

Logging only runs when the mesh layer has a non-empty name.

Call flow (high level)
meshfilter.cpp          → passes mesh name, calls QuadricSimplification
quadric_simp.cpp        → sets up logger, dumps heap after Init, runs loop
tri_edge_collapse_quadric.h → fires event on each Execute()
collapse_logger.h       → writes JSON / JSONL to disk
collapse_event.h        → event struct + callback hook
quadric_simp.h          → MyTriEdgeCollapse::edgeCost() for heap dump
Files by feature
Initial heap (_initial_heap_state_.json)
File	Role
src/meshlabplugins/filter_meshing/meshfilter.cpp
QuadricSimplification(..., m.label().toStdString())
src/meshlabplugins/filter_meshing/quadric_simp.cpp
After Init(), walks DeciSession.h, calls writeInitialHeap()
src/meshlabplugins/filter_meshing/quadric_simp.h
MyTriEdgeCollapse::edgeCost() — reads edge + cost from heap entry
src/common/collapse_logger.h
writeInitialHeap(), fmtCost()
Per-collapse log (_collapse_records.jsonl)
File	Role
src/meshlabplugins/filter_meshing/meshfilter.cpp
Passes mesh name (same as above)
src/meshlabplugins/filter_meshing/quadric_simp.cpp
Sets gOnCollapse() lambda → collapseLogger.write(e)
src/vcglib/vcg/complex/algorithms/local_optimization/collapse_event.h
CollapseEvent struct, gOnCollapse() / gCollapseIdx()
src/vcglib/vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h
Execute() — builds event (edge, cost, faces) before collapse
src/common/collapse_logger.h
write() — one JSON line per collapse
Schema docs
File	Documents
md_files/initial_heap_state_schema.md
Initial heap JSON only
md_files/collapse_records_schema.md
JSONL collapse log (+ pointer to heap schema)
Rebuild reminder
These live in filter_meshing.dll. After pulling changes, rebuild that target (or full Debug build) and copy/use the plugin next to meshlab.exe (build/src/distrib/Debug/plugins/).