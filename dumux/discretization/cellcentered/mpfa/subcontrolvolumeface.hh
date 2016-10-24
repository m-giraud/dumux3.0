// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \brief Class for an MPFA-O sub control volume face
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_SUBCONTROLVOLUMEFACE_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_SUBCONTROLVOLUMEFACE_HH

#include "methods.hh"

namespace Dumux
{
//! Forward declaration of the method specific implementations
//! Available implementations have to be included at the end of this file.
template<MpfaMethods M, class G, typename I>
class CCMpfaSubControlVolumeFaceImplementation;

/*!
 * \ingroup Discretization
 * \brief Class for a sub control volume face in mpfa methods, i.e a part of the boundary
 *        of a control volume we compute fluxes on. This class inherits from the actual implementations
 *        and defines the constructor interface.
 */
template<MpfaMethods M, class G, typename I>
class CCMpfaSubControlVolumeFace : public CCMpfaSubControlVolumeFaceImplementation<M, G, I>
{
    using ParentType = CCMpfaSubControlVolumeFaceImplementation<M, G, I>;
    using Geometry = G;
    using IndexType = I;

    using Scalar = typename Geometry::ctype;
    static const int dim = Geometry::mydimension;
    static const int dimworld = Geometry::coorddimension;

    using GlobalPosition = Dune::FieldVector<Scalar, dimworld>;

public:
    /*!
     * \brief Constructor interface of an mpfa sub control volume face
     *
     * We define a general constructor for scv faces in mpfa methods.
     * This is necessary so that the class instantiating scv faces can work
     * method-independent.
     *
     * \param geomHelper The mpfa geometry helper
     * \param corners The corners of the scv face
     * \param unitOuterNormal The unit outer normal vector of the scvf
     * \param vIdxGlobal The global vertex index the scvf is connected to
     * \param localIndex Some element local index (the local vertex index in mpfao-fps)
     * \param scvfIndex The global index of this scv face
     * \param scvIndices The inside and outside scv indices connected to this face
     * \param q The parameterization of the quadrature point on the scvf for flux calculation
     * \param boundary Boolean to specify whether or not the scvf is on a boundary
     */
    template<class MpfaGeometryHelper>
    CCMpfaSubControlVolumeFace(const MpfaGeometryHelper& geomHelper,
                               std::vector<GlobalPosition>&& corners,
                               GlobalPosition&& unitOuterNormal,
                               IndexType vIdxGlobal,
                               unsigned int localIndex,
                               IndexType scvfIndex,
                               std::array<IndexType, 2>&& scvIndices,
                               Scalar q,
                               bool boundary)
    : ParentType(geomHelper, corners, unitOuterNormal, vIdxGlobal, localIndex, scvfIndex, scvIndices, q, boundary)
    {}
};
} // end namespace

//! The available implementations should be included here
#include <dumux/discretization/cellcentered/mpfa/omethod/subcontrolvolumeface.hh>
#include <dumux/discretization/cellcentered/mpfa/omethodfps/subcontrolvolumeface.hh>

#endif
