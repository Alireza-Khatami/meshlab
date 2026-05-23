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
#include <iomanip>
#include <stdexcept>
#ifdef _WIN32
#  include <direct.h>
#  define CL_MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define CL_MKDIR(p) mkdir((p), 0755)
#endif

class CollapseLogger
{
public:
    CollapseLogger() = default;
    ~CollapseLogger() { close(); }

    // Opens the output file.  Call before starting simplification.
    void open(const std::string& meshName)
    {
        CL_MKDIR("collapse_records"); // no-op if already exists
        const std::string path = "collapse_records/" + meshName + "_collapse_records.jsonl";
        file_.open(path, std::ios::out | std::ios::trunc);
        if (!file_.is_open())
            throw std::runtime_error("CollapseLogger: cannot open " + path);
        file_ << std::fixed << std::setprecision(6);
    }

    void close()
    {
        if (file_.is_open()) file_.close();
    }

    bool isOpen() const { return file_.is_open(); }

    // Write one collapse record.
    void write(const vcg::tri::CollapseEvent& e)
    {
        if (!file_.is_open()) return;

        file_ << '{'
              << "\"idx\":"  << e.idx  << ','
              << "\"cost\":" << e.cost << ','

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
    std::ofstream file_;

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
