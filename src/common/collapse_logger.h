/**
 * collapse_logger.h
 *
 * JSONL writer for QEM edge-collapse events.
 *
 * Output path:  collapse_records/<meshName>_collapse_records.jsonl
 * One JSON object per line, one line per collapse, in collapse order.
 *
 * Each record:
 * {
 *   "idx": 0,
 *   "cost": 0.000123,
 *   "edge": {
 *     "v0": { "id": 42, "pos": [x, y, z] },
 *     "v1": { "id": 37, "pos": [x, y, z] }
 *   },
 *   "new_pos": [x, y, z],
 *   "deleted_faces": [
 *     { "id": 100, "vert_ids": [42,37,5], "verts": [[x,y,z],[x,y,z],[x,y,z]] },
 *     { "id": 101, "vert_ids": [42,37,9], "verts": [[x,y,z],[x,y,z],[x,y,z]] }
 *   ],
 *   "modified_faces": [
 *     { "id": 95,  "vert_ids": [42,11,8], "verts": [[x,y,z],[x,y,z],[x,y,z]] }
 *   ]
 * }
 *
 * Notes:
 *  - deleted_faces  : the 2 triangles sharing the collapsed edge; removed from mesh.
 *  - modified_faces : triangles incident to v0 only; survive with v0 replaced by v1 at new_pos.
 *  - Vertex positions in deleted/modified_faces are PRE-collapse values.
 *  - No new faces or edges are ever created by an edge collapse.
 */

#pragma once

#include <vcg/complex/algorithms/local_optimization/collapse_event.h>

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#ifdef _WIN32
#  include <direct.h>
#  define CL_MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define CL_MKDIR(p) mkdir((p), 0755)
#endif

struct InitialHeapEntry
{
    int    v0_id;
    int    v1_id;
    double cost;
    // Full ComputePriority breakdown (so the QMAT port can verify every stage).
    double opt[3]  = {0, 0, 0};  // optimalPos
    double quadErr = 0;          // ScaleFactor*Apply(opt), after QuadricEpsilon clamp
    double applyOpt= 0;          // raw qq.Apply(optimalPos)
    double applyMid= 0;          // raw qq.Apply(midpoint)
    double gate    = 0;          // Qd(v0).Apply(mid)+Qd(v1).Apply(mid)
    double newQual = 0;          // clamped newQual
    double minCos  = 0;          // clamped+remapped MinCos
};

// One vertex's accumulated InitQuadric quadric (math::Quadric<double>: a[6],b[3],c).
struct VertexQuadricEntry
{
    int    id;
    double pos[3];
    double a[6];
    double b[3];
    double c;
};

class CollapseLogger
{
public:
    CollapseLogger() = default;
    ~CollapseLogger() { close(); }

    // Opens collapse log + remembers mesh name for the initial-heap file.
    void open(const std::string& meshName)
    {
        meshName_ = meshName;
        CL_MKDIR("collapse_records"); // no-op if already exists
        const std::string path = "collapse_records/" + meshName + "_collapse_records.jsonl";
        file_.open(path, std::ios::out | std::ios::trunc);
        if (!file_.is_open())
            throw std::runtime_error("CollapseLogger: cannot open " + path);
    }

    void close()
    {
        if (file_.is_open()) file_.close();
    }

    bool isOpen() const { return file_.is_open(); }

    // All candidate collapses after Init(), sorted by cost ascending (same order as heap pops).
    void writeInitialHeap(const std::vector<InitialHeapEntry>& entries)
    {
        if (meshName_.empty()) return;

        std::vector<InitialHeapEntry> sorted = entries;
        std::sort(sorted.begin(), sorted.end(),
            [](const InitialHeapEntry& a, const InitialHeapEntry& b) { return a.cost < b.cost; });

        const std::string path = "collapse_records/" + meshName_ + "_initial_heap_state_.json";
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            throw std::runtime_error("CollapseLogger: cannot open " + path);

        out << "{\"mesh\":\"" << meshName_ << "\","
            << "\"heap_size\":" << sorted.size() << ","
            << "\"order\":\"ascending_by_cost\","
            << "\"entries\":[";

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            if (i) out << ',';
            const auto& e = sorted[i];
            out << "{\"rank\":" << i
                << ",\"v0_id\":" << e.v0_id
                << ",\"v1_id\":" << e.v1_id
                << ",\"cost\":" << fmtCost(e.cost)
                << ",\"opt\":[" << fmtCost(e.opt[0]) << ',' << fmtCost(e.opt[1]) << ',' << fmtCost(e.opt[2]) << ']'
                << ",\"quadErr\":" << fmtCost(e.quadErr)
                << ",\"applyOpt\":" << fmtCost(e.applyOpt)
                << ",\"applyMid\":" << fmtCost(e.applyMid)
                << ",\"gate\":" << fmtCost(e.gate)
                << ",\"newQual\":" << fmtCost(e.newQual)
                << ",\"minCos\":" << fmtCost(e.minCos)
                << '}';
        }
        out << "]}\n";
    }

    // Per-vertex accumulated quadrics right after InitQuadric (before any collapse).
    // Writes collapse_records/<meshName>_vertex_quadrics_.json.
    void writeVertexQuadrics(const std::vector<VertexQuadricEntry>& verts)
    {
        if (meshName_.empty()) return;
        const std::string path = "collapse_records/" + meshName_ + "_vertex_quadrics_.json";
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            throw std::runtime_error("CollapseLogger: cannot open " + path);

        out << "{\"mesh\":\"" << meshName_ << "\",\"count\":" << verts.size() << ",\"verts\":[";
        for (size_t i = 0; i < verts.size(); ++i)
        {
            if (i) out << ',';
            const auto& v = verts[i];
            out << "{\"id\":" << v.id
                << ",\"pos\":[" << fmtCost(v.pos[0]) << ',' << fmtCost(v.pos[1]) << ',' << fmtCost(v.pos[2]) << ']'
                << ",\"a\":[" << fmtCost(v.a[0]) << ',' << fmtCost(v.a[1]) << ',' << fmtCost(v.a[2]) << ','
                              << fmtCost(v.a[3]) << ',' << fmtCost(v.a[4]) << ',' << fmtCost(v.a[5]) << ']'
                << ",\"b\":[" << fmtCost(v.b[0]) << ',' << fmtCost(v.b[1]) << ',' << fmtCost(v.b[2]) << ']'
                << ",\"c\":" << fmtCost(v.c)
                << '}';
        }
        out << "]}\n";
    }

    // Write one collapse record.
    void write(const vcg::tri::CollapseEvent& e)
    {
        if (!file_.is_open()) return;

        file_ << '{'
              << "\"idx\":"  << e.idx  << ','
              << "\"cost\":" << fmtCost(e.cost) << ','

              << "\"edge\":{"
                << "\"v0\":{\"id\":" << e.v0.id
                  << ",\"pos\":"    << pos3(e.v0.pos) << "},"
                << "\"v1\":{\"id\":" << e.v1.id
                  << ",\"pos\":"    << pos3(e.v1.pos) << "}"
              << "},"

              << "\"new_pos\":"  << pos3(e.new_pos) << ','

              << "\"deleted_faces\":" << faceArray(e.deleted_faces) << ','
              << "\"modified_faces\":" << faceArray(e.modified_faces)

              << "}\n";
    }

private:
    std::string   meshName_;
    std::ofstream file_;

    // Full double precision — avoids flat-region costs printing as 0.000000
    static std::string fmtCost(double x)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", x);
        return buf;
    }

    static std::string pos3(const float p[3])
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "[%.6f,%.6f,%.6f]", p[0], p[1], p[2]);
        return buf;
    }

    static std::string faceArray(const std::vector<vcg::tri::CollapseEvent::Face>& faces)
    {
        std::string s = "[";
        for (size_t i = 0; i < faces.size(); ++i)
        {
            if (i) s += ',';
            const auto& f = faces[i];
            char buf[512];
            std::snprintf(buf, sizeof(buf),
                "{\"id\":%d,"
                "\"vert_ids\":[%d,%d,%d],"
                "\"verts\":["
                  "[%.6f,%.6f,%.6f],"
                  "[%.6f,%.6f,%.6f],"
                  "[%.6f,%.6f,%.6f]"
                "]}",
                f.id,
                f.vert_ids[0], f.vert_ids[1], f.vert_ids[2],
                f.vert_pos[0][0], f.vert_pos[0][1], f.vert_pos[0][2],
                f.vert_pos[1][0], f.vert_pos[1][1], f.vert_pos[1][2],
                f.vert_pos[2][0], f.vert_pos[2][1], f.vert_pos[2][2]);
            s += buf;
        }
        s += ']';
        return s;
    }
};
