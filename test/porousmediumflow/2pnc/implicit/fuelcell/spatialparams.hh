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
 * \ingroup TwoPNCTests
 * \brief Definition of the spatial parameters for the fuel cell
 *        problem which uses the isothermal/non-insothermal 2pnc box model.
 */

#ifndef DUMUX_FUELCELL_SPATIAL_PARAMS_HH
#define DUMUX_FUELCELL_SPATIAL_PARAMS_HH

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedvangenuchten.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

namespace Dumux {
/*!
 * \ingroup TwoPNCTests
 * \brief Definition of the spatial parameters for the FuelCell
 *        problem which uses the isothermal 2p2c box model.
 */
template<class FVGridGeometry, class Scalar>
class FuelCellSpatialParams
: public FVSpatialParams<FVGridGeometry, Scalar,
                         FuelCellSpatialParams<FVGridGeometry, Scalar>>
{
    using GridView = typename FVGridGeometry::GridView;
    using ParentType = FVSpatialParams<FVGridGeometry, Scalar,
                                       FuelCellSpatialParams<FVGridGeometry, Scalar>>;

    static constexpr int dimWorld = GridView::dimensionworld;

    using Element = typename FVGridGeometry::GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    using DimWorldMatrix = Dune::FieldMatrix<Scalar, dimWorld, dimWorld>;

    using EffectiveLaw = RegularizedVanGenuchten<Scalar>;

public:
    using MaterialLaw = EffToAbsLaw<EffectiveLaw>;
    using MaterialLawParams = typename MaterialLaw::Params;
    using PermeabilityType = DimWorldMatrix;

    FuelCellSpatialParams(std::shared_ptr<const FVGridGeometry> fvGridGeometry)
    : ParentType(fvGridGeometry), K_(0)
    {
        // intrinsic permeabilities
        K_[0][0] = 5e-11;
        K_[1][1] = 5e-11;

        // porosities
        porosity_ = 0.2;

        // residual saturations
        materialParams_.setSwr(0.12); // air is wetting phase
        materialParams_.setSnr(0.0); // water is non-wetting

        //parameters for the vanGenuchten law
        materialParams_.setVgAlpha(6.66e-5); // alpha = 1/pcb
        materialParams_.setVgn(3.652);
    }

    /*!
     * \brief Returns the hydraulic conductivity \f$[m^2]\f$
     *
     * \param globalPos The global position
     */
    DimWorldMatrix permeabilityAtPos(const GlobalPosition& globalPos) const
    { return K_; }

    /*!
     * \brief Defines the porosity \f$[-]\f$ of the spatial parameters
     *
     * \param globalPos The global position
     */
    Scalar porosityAtPos(const GlobalPosition& globalPos) const
    {
        if (globalPos[1] < eps_)
            return porosity_;
        else
            return 0.2;
    }

    /*!
     * \brief Returns the parameter object for the Brooks-Corey material law
     * which depends on the position.
     *
     * \param globalPos The global position
     */
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    { return materialParams_; }

    /*!
     * \brief Function for defining which phase is to be considered as the wetting phase.
     *
     * \param globalPos The position of the center of the element
     * \return The wetting phase index
     * \note We set a hydrophobic material.
     */
    template<class FluidSystem>
    int wettingPhaseAtPos(const GlobalPosition& globalPos) const
    { return FluidSystem::gasPhaseIdx; }

private:
    DimWorldMatrix K_;
    Scalar porosity_;
    static constexpr Scalar eps_ = 1e-6;
    MaterialLawParams materialParams_;
};

} // end namespace Dumux

#endif
