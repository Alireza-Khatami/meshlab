
meshfilter.cpp: 991-1032
case FP_QUADRIC_SIMPLIFICATION:
	{
		m.updateDataMask( MeshModel::MM_VERTFACETOPO | MeshModel::MM_VERTMARK);
		tri::UpdateFlags<CMeshO>::FaceBorderFromVF(m.cm);

		int TargetFaceNum = par.getInt("TargetFaceNum");
		if(par.getFloat("TargetPerc")!=0) TargetFaceNum = m.cm.fn*par.getFloat("TargetPerc");

		tri::TriEdgeCollapseQuadricParameter pp;
		pp.QualityThr=lastq_QualityThr =par.getFloat("QualityThr");
		pp.PreserveBoundary=lastq_PreserveBoundary = par.getBool("PreserveBoundary");
		pp.BoundaryQuadricWeight = pp.BoundaryQuadricWeight * par.getFloat("BoundaryWeight");
		pp.PreserveTopology=lastq_PreserveTopology = par.getBool("PreserveTopology");
		pp.QualityWeight=lastq_QualityWeight = par.getBool("QualityWeight");
		pp.NormalCheck=lastq_PreserveNormal = par.getBool("PreserveNormal");
		pp.OptimalPlacement=lastq_OptimalPlacement = par.getBool("OptimalPlacement");
		pp.QualityQuadric=lastq_PlanarQuadric = par.getBool("PlanarQuadric");
		pp.QualityQuadricWeight=lastq_PlanarWeight = par.getFloat("PlanarWeight");
		lastq_Selected = par.getBool("Selected");

		QuadricSimplification(m.cm,TargetFaceNum,lastq_Selected,pp,  cb);

		if(par.getBool("AutoClean"))
		{
			int nullFaces=tri::Clean<CMeshO>::RemoveFaceOutOfRangeArea(m.cm,0);
			if(nullFaces) log( "PostSimplification Cleaning: Removed %d null faces", nullFaces);
			int deldupvert=tri::Clean<CMeshO>::RemoveDuplicateVertex(m.cm);
			if(deldupvert) log( "PostSimplification Cleaning: Removed %d duplicated vertices", deldupvert);
			int delvert=tri::Clean<CMeshO>::RemoveUnreferencedVertex(m.cm);
			if(delvert) log( "PostSimplification Cleaning: Removed %d unreferenced vertices",delvert);
			m.clearDataMask(MeshModel::MM_FACEFACETOPO );
			tri::Allocator<CMeshO>::CompactVertexVector(m.cm);
			tri::Allocator<CMeshO>::CompactFaceVector(m.cm);
		}

		m.updateBoxAndNormals();
		tri::UpdateNormal<CMeshO>::NormalizePerFace(m.cm);
		tri::UpdateNormal<CMeshO>::PerVertexFromCurrentFaceNormal(m.cm);
		tri::UpdateNormal<CMeshO>::NormalizePerVertex(m.cm);

	} break;




quadric_simp.cpp : 28 - 83


void QuadricSimplification(CMeshO &m,int  TargetFaceNum, bool Selected, tri::TriEdgeCollapseQuadricParameter &pp, CallBackPos *cb)
{
  math::Quadric<double> QZero;
  QZero.SetZero();
  tri::QuadricTemp TD(m.vert,QZero);
  tri::QHelper::TDp()=&TD;
  
  if(Selected) // simplify only inside selected faces
  {
    // select only the vertices having ALL incident faces selected
    tri::UpdateSelection<CMeshO>::VertexFromFaceStrict(m);
    
    // Mark not writable un-selected vertices
    for(auto vi=m.vert.begin();vi!=m.vert.end();++vi) if(!(*vi).IsD())
    {
      if(!(*vi).IsS()) (*vi).ClearW();
      else (*vi).SetW();
    }
  }
  
  if(pp.PreserveBoundary && !Selected) 
  {
    pp.FastPreserveBoundary=true;
    pp.PreserveBoundary = false;
  }
  
  if(pp.NormalCheck) pp.NormalThrRad = M_PI/4.0;
  
  vcg::LocalOptimization<CMeshO> DeciSession(m,&pp);
  cb(1,"Initializing simplification");
  DeciSession.Init<tri::MyTriEdgeCollapse >();
  
  if(Selected)
    TargetFaceNum= m.fn - (m.sfn-TargetFaceNum);
  DeciSession.SetTargetSimplices(TargetFaceNum);
  DeciSession.SetTimeBudget(0.1f); // this allows updating the progress bar 10 time for sec...
  //  if(TargetError< numeric_limits<double>::max() ) DeciSession.SetTargetMetric(TargetError);
  //int startFn=m.fn;
  int faceToDel=m.fn-TargetFaceNum;
  while( DeciSession.DoOptimization() && m.fn>TargetFaceNum )
  {
    cb(100-100*(m.fn-TargetFaceNum)/(faceToDel), "Simplifying...");
  };
  
  DeciSession.Finalize<tri::MyTriEdgeCollapse >();
  
  if(Selected) // Clear Writable flags 
  {
    for(auto vi=m.vert.begin();vi!=m.vert.end();++vi) 
    {
      if (!(*vi).IsD()) (*vi).SetW();
      if ((*vi).IsS()) (*vi).ClearS();
    }
  }
  tri::QHelper::TDp()=nullptr;
}


Dependencies to make `quadric_simp.cpp` standalone
--------------------------------------------------

Local workspace files (required):
- src/meshlabplugins/filter_meshing/quadric_simp.cpp
- src/meshlabplugins/filter_meshing/quadric_simp.h
- src/meshlabplugins/filter_meshing/meshfilter.h
- src/common/ml_document/cmesh.h
- src/common/ml_document/cmesh.cpp
- src/common/ml_document/base_types.h
- src/common/ml_document/mesh_document.h
- src/common/ml_document/mesh_document.cpp
- src/common/ml_document/mesh_model.h
- src/common/ml_document/mesh_model.cpp
- src/common/ml_document/raster_model.h
- src/common/ml_document/raster_model.cpp
- src/common/ml_document/helpers/mesh_document_state_data.h
- src/common/ml_document/helpers/mesh_document_state_data.cpp
- src/common/mlexception.h
- src/common/plugins/interfaces/filter_plugin.h
- src/common/plugins/interfaces/filter_plugin.cpp
- src/common/plugins/interfaces/meshlab_plugin.h
- src/common/plugins/interfaces/meshlab_plugin_logger.h
- src/common/plugins/interfaces/meshlab_plugin_logger.cpp

External VCGLib headers (must be available on include path):
- <vcg/complex/complex.h>                                              (pulled in by base_types.h)
- <vcg/container/simple_temporary_data.h>
- <vcg/complex/algorithms/local_optimization.h>
- <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
- <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric_tex.h>
- <vcg/complex/algorithms/update/selection.h>                          (used directly: tri::UpdateSelection<CMeshO>::VertexFromFaceStrict — NOT pulled in transitively)
- <vcg/math/quadric.h>                                                 (provides math::Quadric<double>, pulled in transitively by tri_edge_collapse_quadric.h)

Compile-time requirements:
- MESHLAB_SCALAR must be defined as a compiler flag (e.g. -DMESHLAB_SCALAR=float).
  base_types.h will emit a hard #error if this is missing.

Notes:
- Types required at build time: `CMeshO`, `CVertexO`, `tri::TriEdgeCollapseQuadricParameter`, `CallBackPos`, and the `LocalOptimization` framework.


VCGLib Internals — Full Call Chain
-----------------------------------

### 1. LocalOptimization<CMeshO>  (local_optimization.h)

The driver loop. `quadric_simp.cpp` creates one instance and calls three methods on it:

  DeciSession.Init<MyTriEdgeCollapse>()
    - Calls MyTriEdgeCollapse::Init(m, heap, pp)  (see §2 below)
    - Builds the STL max-heap ordered by collapse error (lowest error = highest priority)

  DeciSession.DoOptimization()           [called in a while loop until fn <= TargetFaceNum]
    - Pops the cheapest (lowest error) HeapElem from the heap
    - Calls locMod->IsUpToDate()  — skips stale entries (vertices modified by a prior collapse)
    - Calls locMod->IsFeasible(pp) — checks topology constraints (see §2)
    - Calls locMod->Execute(m, pp) — performs the collapse (see §2)
    - Calls locMod->UpdateHeap(heap, pp) — re-inserts new candidate collapses around surviving vertex (see §2)
    - If heap grows beyond fn * HeapSimplexRatio, calls ClearHeap() to purge stale entries

  DeciSession.Finalize<MyTriEdgeCollapse>()
    - Calls MyTriEdgeCollapse::Finalize(m, heap, pp) — restores write flags on boundary vertices


### 2. TriEdgeCollapseQuadric  (tri_edge_collapse_quadric.h)

This is the CRTP policy class that MyTriEdgeCollapse (defined in quadric_simp.h) inherits from.
It implements all five methods the LocalOptimization loop calls:

#### Init(m, heap, pp)   [static, called once]
  1. Calls vcg::tri::UpdateTopology<CMeshO>::VertexFace(m)  — builds VF adjacency
  2. Calls vcg::tri::UpdateFlags<CMeshO>::FaceBorderFromVF(m) — marks border edges/vertices
  3. Marks boundary vertices non-writable if FastPreserveBoundary or PreserveBoundary is set
  4. Calls InitQuadric(m, pp):
       - Zeroes per-vertex quadrics via QHelper::Qd(v).SetZero()
       - For every non-deleted face:
           * Builds a Plane3 from the face normal (area-weighted if pp->UseArea)
           * Calls q.ByPlane(facePlane) to get a 4x4 quadric matrix for that plane
           * Accumulates q into each of the 3 face vertices: QHelper::Qd(v) += q
           * For each border edge (or if pp->QualityQuadric):
               builds an orthogonal "border plane" through the edge,
               weighted by pp->BoundaryQuadricWeight (border) or pp->QualityQuadricWeight (quality),
               accumulates that border quadric into the two edge endpoints
       - If pp->ScaleIndependent: computes ScaleFactor = 1e8 / bbox.Diag()^6  (makes error mesh-size independent)
       - If pp->QualityWeight: scales each vertex quadric by a factor derived from its quality attribute
  5. Enumerates all candidate edge collapses and pushes MyTriEdgeCollapse objects onto the heap
     (symmetric if OptimalPlacement is on, asymmetric otherwise)

#### ComputePriority(pp)   [called once per candidate at construction, result stored as _priority]
  1. Calls ComputePosition(pp) — computes optimalPos (see below)
  2. Temporarily moves v0 and v1 to optimalPos to simulate the collapse
  3. Collects optional pre-collapse data: original face normals (NormalCheck), area (AreaCheck), quality (HardQualityCheck)
  4. Computes quadric error:   QuadErr = ScaleFactor * (Qd(v0) + Qd(v1)).Apply(optimalPos)
  5. Computes post-collapse quality of surviving faces (QualityCheck)
  6. Computes min cosine of normal deviation across surviving faces (NormalCheck)
  7. Final error = QuadErr / (newQual * MinCos)  — divided by whichever checks are enabled
     (infinity if AreaCheck or HardQualityCheck or HardNormalCheck constraints are violated)
  8. Restores v0 and v1 to original positions

#### ComputePosition(pp)   [called inside ComputePriority]
  - If OptimalPlacement == false: new position = v1->P()  (non-optimal, v0 collapses to v1)
  - If OptimalPlacement == true:
      * Sums quadrics: q = Qd(v0) + Qd(v1)
      * If SVDPlacement: calls q.MinimumClosestToPoint(x, midpoint)  (SVD-based minimum)
      * Otherwise:       calls q.Minimum(x)  — solves the 3x3 linear system A*x = b analytically
      * Sets optimalPos = x

#### IsFeasible(pp)
  - If PreserveTopology == false: always returns true (no check)
  - If PreserveTopology == true:  calls EdgeCollapser::LinkConditions(pos)
      * Checks the link condition: the edge can be collapsed without changing topology
        (i.e. link(v0) ∩ link(v1) == link(edge(v0,v1)))

#### Execute(m, pp)
  1. QHelper::Qd(v1) += QHelper::Qd(v0)  — accumulate deleted vertex quadric into surviving vertex
  2. EdgeCollapser<CMeshO,VertexPair>::Do(m, pos, optimalPos)
       - Moves v1 to optimalPos
       - Deletes v0 and the two incident degenerate faces
       - Relinks all VF adjacency pointers

#### UpdateHeap(heap, pp)
  - Increments GlobalMark (invalidates all existing heap entries touching v1's neighbourhood)
  - Walks the VF star of the surviving vertex v1
  - For each neighbouring vertex that is still writable and not yet visited:
      pushes a new MyTriEdgeCollapse(VertexPair(v0, v1), ...) onto the heap via AddCollapseToHeap


### 3. math::Quadric<double>  (vcg/math/quadric.h — included transitively)

Stores a symmetric 4×4 matrix (10 doubles) representing the squared distance to a set of planes.

Key methods used:
  q.SetZero()           — zero-initialise
  q.ByPlane(plane)      — set quadric from a Plane3:  Q = n*nT, offset terms included
  q += other            — accumulate quadrics (sum of squared distances)
  q.Apply(point)        — evaluate error: xT * A * x + 2*b*x + c
  q.Minimum(x)          — solve A*x = -b for the position of minimum error (returns false if A is singular)
