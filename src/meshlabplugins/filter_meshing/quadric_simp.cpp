/****************************************************************************
 * MeshLab                                                           o o     *
 * A versatile mesh processing toolbox                             o     o   *
 *                                                                _   O  _   *
 * Copyright(C) 2005                                                \/)\/    *
 * Visual Computing Lab                                            /\/|      *
 * ISTI - Italian National Research Council                           |      *
 *                                                                    \      *
 * All rights reserved.																											 *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
 * for more details.                                                         *
 *                                                                           *
 ****************************************************************************/
#include "meshfilter.h"
#include "quadric_simp.h"
#include <common/collapse_logger.h>

using namespace vcg;
using namespace std;

void QuadricSimplification(CMeshO &m, int TargetFaceNum, bool Selected, tri::TriEdgeCollapseQuadricParameter &pp, CallBackPos *cb, const std::string& meshName)
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
  
  // --- Set up collapse logger if a mesh name was provided ---
  CollapseLogger collapseLogger;
  if (!meshName.empty())
  {
    collapseLogger.open(meshName);
    vcg::tri::gCollapseIdx() = 0;
    vcg::tri::gOnCollapse()  = [&collapseLogger](const vcg::tri::CollapseEvent& e)
    {
      collapseLogger.write(e);
    };
  }
  // -----------------------------------------------------------

  vcg::LocalOptimization<CMeshO> DeciSession(m,&pp);
  cb(1,"Initializing simplification");
  DeciSession.Init<tri::MyTriEdgeCollapse >();

  if (!meshName.empty())
  {
    std::vector<InitialHeapEntry> heapEntries;
    heapEntries.reserve(DeciSession.h.size());
    for (const auto& he : DeciSession.h)
    {
      auto* collapse = dynamic_cast<tri::MyTriEdgeCollapse*>(he.locModPtr);
      if (!collapse) continue;
      InitialHeapEntry entry;
      collapse->edgeCost(m, entry.v0_id, entry.v1_id, entry.cost, entry.opt,
                         entry.quadErr, entry.applyOpt, entry.applyMid,
                         entry.gate, entry.newQual, entry.minCos);
      heapEntries.push_back(entry);
    }
    collapseLogger.writeInitialHeap(heapEntries);

    // Dump the per-vertex accumulated quadrics (post-InitQuadric, pre-collapse)
    // so the QMAT port can verify InitQuadric vertex-by-vertex.
    std::vector<VertexQuadricEntry> vqs;
    vqs.reserve(m.vert.size());
    for (auto vi = m.vert.begin(); vi != m.vert.end(); ++vi)
    {
      if ((*vi).IsD()) continue;
      const vcg::math::Quadric<double>& q = tri::QHelper::Qd(*vi);
      VertexQuadricEntry ve;
      ve.id = static_cast<int>(vcg::tri::Index(m, *vi));
      ve.pos[0] = (*vi).P()[0]; ve.pos[1] = (*vi).P()[1]; ve.pos[2] = (*vi).P()[2];
      for (int k = 0; k < 6; ++k) ve.a[k] = q.a[k];
      for (int k = 0; k < 3; ++k) ve.b[k] = q.b[k];
      ve.c = q.c;
      vqs.push_back(ve);
    }
    collapseLogger.writeVertexQuadrics(vqs);
  }

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

  // --- Tear down logger ---
  vcg::tri::gOnCollapse() = nullptr;
  collapseLogger.close();
  // ------------------------
  
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



void QuadricTexSimplification(CMeshO &m,int  TargetFaceNum, bool Selected, tri::TriEdgeCollapseQuadricTexParameter &pp, CallBackPos *cb)
{
  tri::UpdateNormal<CMeshO>::PerFace(m);
	math::Quadric<double> QZero;
	QZero.SetZero();
  tri::QuadricTexHelper<CMeshO>::QuadricTemp TD3(m.vert,QZero);
  tri::QuadricTexHelper<CMeshO>::TDp3()=&TD3;

  std::vector<std::pair<vcg::TexCoord2<float>,Quadric5<double> > > qv;

  tri::QuadricTexHelper<CMeshO>::Quadric5Temp TD(m.vert,qv);
  tri::QuadricTexHelper<CMeshO>::TDp()=&TD;

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
	
  vcg::LocalOptimization<CMeshO> DeciSession(m,&pp);
	cb(1,"Initializing simplification");
	DeciSession.Init<tri::MyTriEdgeCollapseQTex>();

	if(Selected)
		TargetFaceNum= m.fn - (m.sfn-TargetFaceNum);
	DeciSession.SetTargetSimplices(TargetFaceNum);
	DeciSession.SetTimeBudget(0.1f);
//	int startFn=m.fn;
  
	int faceToDel=m.fn-TargetFaceNum;
	
	while( DeciSession.DoOptimization() && m.fn>TargetFaceNum )
	{
    char buf[256];
    sprintf(buf,"Simplifing: heap size %i ops %i\n",int(DeciSession.h.size()),DeciSession.nPerformedOps);
	   cb(100-100*(m.fn-TargetFaceNum)/(faceToDel), buf);
	};

	DeciSession.Finalize<tri::MyTriEdgeCollapseQTex>();
	
	if(Selected) // Clear Writable flags 
  {
    for (auto vi = m.vert.begin(); vi != m.vert.end(); ++vi)
	{
		if (!(*vi).IsD()) (*vi).SetW();
		if ((*vi).IsS()) (*vi).ClearS();
	}
  }
	
  tri::QuadricTexHelper<CMeshO>::TDp3()=nullptr;
  tri::QuadricTexHelper<CMeshO>::TDp()=nullptr;

}
