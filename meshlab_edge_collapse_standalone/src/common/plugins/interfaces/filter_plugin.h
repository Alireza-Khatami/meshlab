/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005-2020                                           \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
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

#ifndef MESHLAB_FILTER_PLUGIN_H
#define MESHLAB_FILTER_PLUGIN_H

#include "meshlab_plugin_logger.h"
#include "meshlab_plugin.h"
#include "../../ml_document/mesh_document.h"

//declaring types to be used as QVariants

Q_DECLARE_METATYPE(Point2m)
Q_DECLARE_METATYPE(Point3m)
Q_DECLARE_METATYPE(Box3m)
Q_DECLARE_METATYPE(Matrix33m)
Q_DECLARE_METATYPE(Matrix44m)
Q_DECLARE_METATYPE(Eigen::VectorXd)

/**
 * @brief The FilterPlugin class provide the interface of the filter plugins.
 */
class FilterPlugin : virtual public MeshLabPlugin, virtual public MeshLabPluginLogger
{
public:
	/** 
	 * @brief The FilterClass enum represents the set of keywords that must be used to categorize a filter.
	 * Each filter can belong to one or more filtering class, or-ed together.
	 */
	enum FilterClass
	{
		Generic        = 0x00000, /*!< Should be avoided if possible. */  //
		Selection      = 0x00001, /*!<  select or de-select something, basic operation on selections (like deleting)*/
		Cleaning       = 0x00002, /*!<  Filters that can be used to clean meshes (duplicated vertices etc)*/
		Remeshing      = 0x00004, /*!<  Simplification, Refinement, Reconstruction and mesh optimization*/
		FaceColoring   = 0x00008,
		VertexColoring = 0x00010,
		MeshColoring   = 0x00020,
		MeshCreation   = 0x00040,
		Smoothing      = 0x00080, /*!<  Stuff that does not change the topology, but just the vertex positions*/
		Quality        = 0x00100,
		Layer          = 0x00200, /*!<  Layers, attributes */
		RasterLayer    = 0x00400, /*!<  Raster Layers, attributes */
		Normal         = 0x00800, /*!<  Normal, Curvature, orientation (rotations and transformations fall here)*/
		Sampling       = 0x01000,
		Texture        = 0x02000,
		RangeMap       = 0x04000, /*!<  filters specific for range map processing*/
		PointSet       = 0x08000,
		Measure        = 0x10000, /*!<  Filters that compute measures and information on meshes.*/
		Polygonal      = 0x20000, /*!<  Filters that works on polygonal and quad meshes.*/
		Camera         = 0x40000, /*!<  Filters that works on shot of mesh and raster.*/
		Other          = 0x80000
	};






