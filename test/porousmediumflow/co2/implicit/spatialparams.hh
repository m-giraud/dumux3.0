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
 * \ingroup CO2Tests
 * \brief Definition of the spatial parameters for the heterogeneous
 *        problem which uses the non-isothermal or isothermal CO2
 *        fully implicit model.
 */

#ifndef DUMUX_HETEROGENEOUS_SPATIAL_PARAMS_HH
#define DUMUX_HETEROGENEOUS_SPATIAL_PARAMS_HH

#include <dumux/io/grid/griddata.hh>
#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

#include <dumux/porousmediumflow/co2/model.hh>

namespace Dumux {

/*!
 * \ingroup CO2Tests
 * \brief Definition of the spatial parameters for the heterogeneous
 *        problem which uses the non-isothermal or isothermal CO2
 *        fully implicit model.
 */
template<class FVGridGeometry, class Scalar>
class HeterogeneousSpatialParams
: public FVSpatialParams<FVGridGeometry,
                         Scalar,
                         HeterogeneousSpatialParams<FVGridGeometry, Scalar>>
{
    using Grid = typename FVGridGeometry::Grid;
    using GridView = typename FVGridGeometry::GridView;
    using FVElementGeometry = typename FVGridGeometry::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using Element = typename GridView::template Codim<0>::Entity;
    using ParentType = FVSpatialParams<FVGridGeometry, Scalar, HeterogeneousSpatialParams<FVGridGeometry, Scalar>>;

    using GlobalPosition = typename SubControlVolume::GlobalPosition;

    using EffectiveLaw = RegularizedBrooksCorey<Scalar>;

public:
    using MaterialLaw = EffToAbsLaw<EffectiveLaw>;
    using MaterialLawParams = typename MaterialLaw::Params;
    using PermeabilityType = Scalar;

    HeterogeneousSpatialParams(std::shared_ptr<const FVGridGeometry> fvGridGeometry,
                               std::shared_ptr<const GridData<Grid>> gridData)
    : ParentType(fvGridGeometry), gridData_(gridData)
    {

        // Set the permeability for the layers
        barrierTopK_ = 1e-17; //sqm
        barrierMiddleK_ = 1e-15; //sqm
        reservoirK_ = 1e-14; //sqm

        // Set the effective porosity of the layers
        barrierTopPorosity_ = 0.001;
        barrierMiddlePorosity_ = 0.05;
        reservoirPorosity_ = 0.2;

        // Same material parameters for every layer
        materialParams_.setSwr(0.2);
        materialParams_.setSwr(0.05);
        materialParams_.setLambda(2.0);
        materialParams_.setPe(1e4);
    }

    /*!
     * \brief Reads layer information from the grid
     */
    void getParamsFromGrid()
    {
        const auto& gridView = this->fvGridGeometry().gridView();
        paramIdx_.resize(gridView.size(0));

        for (const auto& element : elements(gridView))
        {
            const auto eIdx = this->fvGridGeometry().elementMapper().index(element);
            paramIdx_[eIdx] = gridData_->parameters(element)[0];
        }
    }

    /*!
     * \brief Function for defining the (intrinsic) permeability \f$[m^2]\f$
     * \note  It is possibly solution dependent.
     *
     * \param element The current element
     * \param scv The sub-control volume inside the element.
     * \param elemSol The solution at the dofs connected to the element.
     *
     * \return The instrinsic permeability
     */
    template<class ElementSolution>
    PermeabilityType permeability(const Element& element,
                                  const SubControlVolume& scv,
                                  const ElementSolution& elemSol) const
    {
        // Get the global index of the element
        const auto eIdx = this->fvGridGeometry().elementMapper().index(element);
        return permeability(eIdx);
    }

    /*!
     * \brief Function for defining the (intrinsic) permeability \f$[m^2]\f$
     * \note  It is possibly solution dependent.
     *
     * \param eIdx The element index
     */
    PermeabilityType permeability(std::size_t eIdx) const
    {
        if (paramIdx_[eIdx] == barrierTop_)
            return barrierTopK_;
        else if (paramIdx_[eIdx] == barrierMiddle_)
            return barrierMiddleK_;
        else
            return reservoirK_;
    }

    /*!
     * \brief Returns the volume fraction of the inert component with index compIdx \f$[-]\f$
     *
     * \param element The current element
     * \param scv The sub-control volume inside the element.
     * \param elemSol The element solution
     * \param compIdx The solid component index
     * \return The solid volume fraction
     */
    template<class SolidSystem, class ElementSolution>
    Scalar inertVolumeFraction(const Element& element,
                               const SubControlVolume& scv,
                               const ElementSolution& elemSol,
                               int compIdx) const
    {
        // Get the global index of the element
        const auto eIdx = this->fvGridGeometry().elementMapper().index(element);
        return inertVolumeFraction(eIdx);
    }

    /*!
     * \brief Returns the porosity \f$[-]\f$
     *
     * \param eIdx The element index
     */
    Scalar inertVolumeFraction(std::size_t eIdx) const
    {
        if (paramIdx_[eIdx] == barrierTop_)
            return 1- barrierTopPorosity_;
        else if (paramIdx_[eIdx] == barrierMiddle_)
            return 1- barrierMiddlePorosity_;
        else
            return 1- reservoirPorosity_;

    }


    /*!
     * \brief Function for defining the parameters needed by constitutive relationships (kr-sw, pc-sw, etc.).
     *
     * \param globalPos The position of the center of the element
     * \return The material parameters object
     */
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    {
        return materialParams_;
    }

    /*!
     * \brief Function for defining which phase is to be considered as the wetting phase.
     *
     * \param globalPos The position of the center of the element
     * \return The wetting phase index
     */
    template<class FluidSystem>
    int wettingPhaseAtPos(const GlobalPosition& globalPos) const
    { return FluidSystem::BrineIdx; }

private:

    std::shared_ptr<const GridData<Grid>> gridData_;

    int barrierTop_ = 1;
    int barrierMiddle_ = 2;
    int reservoir_ = 3;

    Scalar barrierTopPorosity_;
    Scalar barrierMiddlePorosity_;
    Scalar reservoirPorosity_;

    Scalar barrierTopK_;
    Scalar barrierMiddleK_;
    Scalar reservoirK_;

    MaterialLawParams materialParams_;
    std::vector<int> paramIdx_;
};

} // end namespace Dumux

#endif
