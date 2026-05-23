/**
 * collapse_event.h
 *
 * Defines CollapseEvent (the data captured for one edge collapse) and the
 * global callback hook used by TriEdgeCollapseQuadric::Execute() to fire
 * per-collapse notifications without coupling VCGLib to any specific logger.
 *
 * Usage:
 *   vcg::tri::gOnCollapse = [](const vcg::tri::CollapseEvent& e){ ... };
 *   vcg::tri::gCollapseIdx = 0;
 *   // run simplification ...
 *   vcg::tri::gOnCollapse = nullptr;
 */

#pragma once
#include <functional>
#include <vector>
#include <array>

namespace vcg {
namespace tri {

struct CollapseEvent
{
    // Ordinal position of this collapse in the sequence (0-based)
    int idx;

    // QEM error / priority assigned to this collapse by ComputePriority()
    double cost;

    // The edge being collapsed: v0 is deleted, v1 survives at new_pos
    struct Vertex {
        int   id;
        float pos[3];
    };
    Vertex v0;
    Vertex v1;
    float  new_pos[3]; // optimal position v1 moves to

    // Faces that share the collapsed edge — they become degenerate and are deleted.
    // An edge collapse always removes exactly 2 faces (for a manifold mesh).
    struct Face {
        int   id;
        // Vertex ids and coordinates of each corner, in face winding order
        int   vert_ids[3];
        float vert_pos[3][3];
    };
    std::vector<Face> deleted_faces;   // the 2 faces that are removed

    // Faces that were incident to v0 but did NOT share the collapsed edge.
    // These survive: their v0 corner reference is replaced by v1 at new_pos.
    // Coordinates here are the PRE-collapse values.
    std::vector<Face> modified_faces;
};

// Global callback — set before Init(), clear after Finalize().
// Implemented as inline functions returning static locals to stay C++11/14 compatible
// (inline variables are C++17 only).
// Thread-safety: not provided; use from a single thread.
inline std::function<void(const CollapseEvent&)>& gOnCollapse()
{
    static std::function<void(const CollapseEvent&)> cb;
    return cb;
}

// Running counter — reset to 0 in QuadricSimplification before each run.
inline int& gCollapseIdx()
{
    static int idx = 0;
    return idx;
}

} // namespace tri
} // namespace vcg
