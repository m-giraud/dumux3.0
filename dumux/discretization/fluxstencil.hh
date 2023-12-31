// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \ingroup Discretization
 * \brief The flux stencil specialized for different discretization schemes
 */
#ifndef DUMUX_DISCRETIZATION_FLUXSTENCIL_HH
#define DUMUX_DISCRETIZATION_FLUXSTENCIL_HH

#include <vector>

#include <dune/common/reservedvector.hh>
#include <dumux/common/indextraits.hh>
#include <dumux/discretization/method.hh>

namespace Dumux {

/*!
 * \ingroup Discretization
 * \brief The flux stencil specialized for different discretization schemes
 * \note There might be different stencils used for e.g. advection and diffusion for schemes
 *       where the stencil depends on variables. Also schemes might even have solution dependent
 *       stencil. However, we always reserve the stencil or all DOFs that are possibly involved
 *       since we use the flux stencil for matrix and assembly. This might lead to some zeros stored
 *       in the matrix.
 */
template<class FVElementGeometry, DiscretizationMethod discMethod = FVElementGeometry::FVGridGeometry::discMethod>
class FluxStencil;

/*
 * \ingroup Discretization
 * \brief Flux stencil specialization for the cell-centered tpfa scheme
 * \tparam FVElementGeometry The local view on the finite volume grid geometry
 */
template<class FVElementGeometry>
class FluxStencil<FVElementGeometry, DiscretizationMethod::cctpfa>
{
    using FVGridGeometry = typename FVElementGeometry::FVGridGeometry;
    using SubControlVolumeFace = typename FVGridGeometry::SubControlVolumeFace;
    using GridView = typename FVGridGeometry::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using GridIndexType = typename IndexTraits<GridView>::GridIndex;

public:
    //! Each cell I couples to a cell J always only via one face
    using ScvfStencilIForJ = Dune::ReservedVector<GridIndexType, 1>;

    //! The flux stencil type
    using Stencil = typename SubControlVolumeFace::Traits::GridIndexStorage;

    //! Returns the flux stencil
    static Stencil stencil(const Element& element,
                           const FVElementGeometry& fvGeometry,
                           const SubControlVolumeFace& scvf)
    {
        if (scvf.boundary())
            return Stencil({scvf.insideScvIdx()});
        else if (scvf.numOutsideScvs() > 1)
        {
            Stencil stencil({scvf.insideScvIdx()});
            for (unsigned int i = 0; i < scvf.numOutsideScvs(); ++i)
                stencil.push_back(scvf.outsideScvIdx(i));
            return stencil;
        }
        else
            return Stencil({scvf.insideScvIdx(), scvf.outsideScvIdx()});
    }
};

/*
 * \ingroup Discretization
 * \brief Flux stencil specialization for the cell-centered mpfa scheme
 * \tparam FVElementGeometry The local view on the finite volume grid geometry
 */
template<class FVElementGeometry>
class FluxStencil<FVElementGeometry, DiscretizationMethod::ccmpfa>
{
    using FVGridGeometry = typename FVElementGeometry::FVGridGeometry;
    using SubControlVolumeFace = typename FVGridGeometry::SubControlVolumeFace;
    using GridView = typename FVGridGeometry::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using GridIndexType = typename IndexTraits<GridView>::GridIndex;

    // Use the stencil type of the primary interaction volume
    using NodalIndexSet = typename FVGridGeometry::GridIVIndexSets::DualGridIndexSet::NodalIndexSet;

public:
    //! We don't know yet how many faces couple to a neighboring element
    using ScvfStencilIForJ = std::vector<GridIndexType>;

    //! The flux stencil type
    using Stencil = typename NodalIndexSet::NodalGridStencilType;

    //! Returns the indices of the elements required for flux calculation on an scvf.
    static const Stencil& stencil(const Element& element,
                                  const FVElementGeometry& fvGeometry,
                                  const SubControlVolumeFace& scvf)
    {
        const auto& fvGridGeometry = fvGeometry.fvGridGeometry();

        // return the scv (element) indices in the interaction region
        if (fvGridGeometry.vertexUsesSecondaryInteractionVolume(scvf.vertexIndex()))
            return fvGridGeometry.gridInteractionVolumeIndexSets().secondaryIndexSet(scvf).gridScvIndices();
        else
            return fvGridGeometry.gridInteractionVolumeIndexSets().primaryIndexSet(scvf).gridScvIndices();
    }
};

} // end namespace Dumux

#endif
