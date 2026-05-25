# Collapse Records Schema

## Overview

Each simplification run of **Simplification: Quadric Edge Collapse Decimation** produces two files:

```
collapse_records/<meshName>_initial_heap_state_.json   # all candidates before any collapse
collapse_records/<meshName>_collapse_records.jsonl     # one line per performed collapse
```

The JSONL file has one self-contained JSON object per line, one line per collapse, written in the exact order the collapses were performed.

**Cost precision:** `cost` values use `%.17g` (full `double` precision), not 6 decimal places, so very small flat-region errors are not rounded to `0.000000`.

---

## Key Facts About Edge Collapse Topology

- An edge collapse **removes** 1 vertex (`v0`), 2 faces (the two triangles sharing the collapsed edge), and their associated implicit edges.
- An edge collapse **never creates** new faces or edges.
- Faces that were incident to `v0` but did **not** share the collapsed edge survive. Their `v0` corner reference is replaced in-place by `v1` moved to `new_pos`. These are called **modified faces**.
- All vertex positions logged in `deleted_faces` and `modified_faces` are **pre-collapse** values.

---

## Initial Heap File (`<meshName>_initial_heap_state_.json`)

Full schema for this file: **`md_files/initial_heap_state_schema.md`**

Written once immediately after `Init()`, before the first collapse. Entries are sorted by **ascending cost** (same order the optimizer will pop from the heap).

```json
{
  "mesh": "bunny",
  "heap_size": 150000,
  "order": "ascending_by_cost",
  "entries": [
    { "rank": 0, "v0_id": 42, "v1_id": 37, "cost": 1.23456789012345e-12 },
    { "rank": 1, "v0_id": 10, "v1_id": 11, "cost": 2.34567890123456e-12 }
  ]
}
```

| Field | Type | Description |
|-------|------|-------------|
| `mesh` | `string` | Mesh label passed from MeshLab |
| `heap_size` | `int` | Number of candidate edge collapses in the initial heap |
| `order` | `string` | Always `"ascending_by_cost"` |
| `entries[].rank` | `int` | 0-based rank after sorting (0 = cheapest collapse) |
| `entries[].v0_id` | `int` | Vertex that would be deleted if this collapse runs |
| `entries[].v1_id` | `int` | Vertex that would survive |
| `entries[].cost` | `float` | QEM priority / error for this edge |

---

## Collapse Record Structure (JSONL)

```json
{
  "idx":       <integer>,
  "cost":      <float>,
  "edge": {
    "v0": { "id": <integer>, "pos": [<float>, <float>, <float>] },
    "v1": { "id": <integer>, "pos": [<float>, <float>, <float>] }
  },
  "new_pos":   [<float>, <float>, <float>],
  "deleted_faces":  [ <Face>, ... ],
  "modified_faces": [ <Face>, ... ]
}
```

### Face object

```json
{
  "id":       <integer>,
  "vert_ids": [<integer>, <integer>, <integer>],
  "verts":    [
    [<float>, <float>, <float>],
    [<float>, <float>, <float>],
    [<float>, <float>, <float>]
  ]
}
```

---

## Field Reference

### Top-level fields

| Field | Type | Description |
|-------|------|-------------|
| `idx` | `int` | Zero-based ordinal of this collapse in the sequence |
| `cost` | `float` | QEM error value assigned by `ComputePriority()`. Lower = cheaper collapse. |
| `edge.v0` | `Vertex` | The vertex that is **deleted**. `id` is its global index in the mesh vertex array. |
| `edge.v1` | `Vertex` | The vertex that **survives**. After the collapse it is moved to `new_pos`. |
| `new_pos` | `[x,y,z]` | The optimal position `v1` is moved to (result of `q.Minimum()` or midpoint). |
| `deleted_faces` | `Face[]` | The 2 triangles that share the collapsed edge. Always exactly 2 for a manifold mesh. |
| `modified_faces` | `Face[]` | Triangles incident to `v0` only. Survive with their `v0` corner repointed to `v1` at `new_pos`. |

### Vertex object

| Field | Type | Description |
|-------|------|-------------|
| `id` | `int` | Global vertex index (position in the mesh vertex array) |
| `pos` | `[x,y,z]` | World-space coordinates **before** the collapse |

### Face object

| Field | Type | Description |
|-------|------|-------------|
| `id` | `int` | Global face index (position in the mesh face array) |
| `vert_ids` | `[int,int,int]` | Global vertex indices of the 3 corners, in face winding order |
| `verts` | `[[x,y,z],[x,y,z],[x,y,z]]` | World-space coordinates of each corner, **pre-collapse**, matching `vert_ids` order |

---

## Example Record

```json
{"idx":0,"cost":0.000042,"edge":{"v0":{"id":142,"pos":[0.123456,1.234567,2.345678]},"v1":{"id":87,"pos":[0.234567,1.345678,2.456789]}},"new_pos":[0.178901,1.289012,2.400000],"deleted_faces":[{"id":300,"vert_ids":[142,87,55],"verts":[[0.123456,1.234567,2.345678],[0.234567,1.345678,2.456789],[0.500000,1.500000,2.500000]]},{"id":301,"vert_ids":[142,87,99],"verts":[[0.123456,1.234567,2.345678],[0.234567,1.345678,2.456789],[0.600000,1.600000,2.600000]]}],"modified_faces":[{"id":298,"vert_ids":[142,44,21],"verts":[[0.123456,1.234567,2.345678],[0.111111,1.111111,2.111111],[0.222222,2.222222,2.222222]]}]}
```

---

## Reconstructing the Collapse Sequence

To replay the simplification from the log:

1. Start with the original mesh.
2. For each record in order (`idx` 0, 1, 2 …):
   a. Remove vertex `edge.v0.id` from the vertex list.
   b. Remove faces listed in `deleted_faces` (by `id`).
   c. For each face in `modified_faces`: replace the corner whose `vert_id == edge.v0.id` with `edge.v1.id` and update its position to `new_pos`.
3. The mesh state after all records matches the simplified output.

---

## Parsing (Python example)

```python
import json
import pandas as pd

records = []
with open("collapse_records/bunny_collapse_records.jsonl") as f:
    for line in f:
        records.append(json.loads(line))

# Flat view of collapse costs in order
costs = [r["cost"] for r in records]

# Load into a DataFrame (top-level scalar fields only)
df = pd.DataFrame([
    {
        "idx":     r["idx"],
        "cost":    r["cost"],
        "v0_id":   r["edge"]["v0"]["id"],
        "v1_id":   r["edge"]["v1"]["id"],
        "new_x":   r["new_pos"][0],
        "new_y":   r["new_pos"][1],
        "new_z":   r["new_pos"][2],
        "n_deleted":  len(r["deleted_faces"]),
        "n_modified": len(r["modified_faces"]),
    }
    for r in records
])
```

---

## Source Files

| File | Role |
|------|------|
| `src/vcglib/vcg/complex/algorithms/local_optimization/collapse_event.h` | `CollapseEvent` struct and `gOnCollapse` callback |
| `src/vcglib/vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h` | `Execute()` — fires the callback with collected face data before collapse |
| `src/common/collapse_logger.h` | JSONL writer (`CollapseLogger`) |
| `src/meshlabplugins/filter_meshing/quadric_simp.cpp` | Wires `CollapseLogger` into the optimization loop |
| `src/meshlabplugins/filter_meshing/meshfilter.cpp` | Passes mesh label as `meshName` to `QuadricSimplification` |
