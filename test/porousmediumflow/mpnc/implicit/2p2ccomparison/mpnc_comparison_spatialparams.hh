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
 * \ingroup TwoPTwoCTests
 * \brief The spatial parameters for the TwoPTwoC MPNC comparison problem
 */

#ifndef DUMUX_MPNC_COMPARISON_SPATIAL_PARAMS_HH
#define DUMUX_MPNC_COMPARISON_SPATIAL_PARAMS_HH

#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedlinearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

#include <dumux/material/fluidmatrixinteractions/mp/2padapter.hh>

#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>

namespace Dumux {

/*!
 * \ingroup MPNCTests
 * \brief The spatial parameters for the ObstacleProblem
 */
//forward declaration
template<class TypeTag>
class MPNCComparisonSpatialParams;

namespace Properties {

// The spatial parameters TypeTag
NEW_TYPE_TAG(MPNCComparisonSpatialParams);

// Set the spatial parameters
SET_TYPE_PROP(MPNCComparisonSpatialParams, SpatialParams, MPNCComparisonSpatialParams<TypeTag>);

// Set the material Law
SET_PROP(MPNCComparisonSpatialParams, MaterialLaw)
{
private:
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    enum {wPhaseIdx = FluidSystem::wPhaseIdx};
    // define the material law
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using EffMaterialLaw = RegularizedBrooksCorey<Scalar>;
    using TwoPMaterialLaw = EffToAbsLaw<EffMaterialLaw>;
public:
    using type = TwoPAdapter<wPhaseIdx, TwoPMaterialLaw>;
};
}

/**
 * \ingroup MPNCModel
 * \ingroup ImplicitTestProblems
 * \brief Definition of the spatial params properties for the obstacle problem
 *
 */
template<class TypeTag>
class MPNCComparisonSpatialParams : public FVSpatialParams<TypeTag>
{
    using ParentType = FVSpatialParams<TypeTag>;
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry)::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using GlobalPosition = Dune::FieldVector<Scalar, GridView::dimension>;

    enum {dimWorld=GridView::dimensionworld};

public:
    using PermeabilityType = Scalar;
    using MaterialLaw = typename GET_PROP_TYPE(TypeTag, MaterialLaw);
    using MaterialLawParams = typename MaterialLaw::Params;

    //! The constructor
    MPNCComparisonSpatialParams(const Problem &problem) : ParentType(problem)
    {
        // intrinsic permeabilities
        coarseK_ = 1e-12;
        fineK_ = 1e-15;

        // the porosity
        porosity_ = 0.3;

        // residual saturations
        fineMaterialParams_.setSwr(0.2);
        fineMaterialParams_.setSnr(0.0);
        coarseMaterialParams_.setSwr(0.2);
        coarseMaterialParams_.setSnr(0.0);

        // parameters for the Brooks-Corey law
        fineMaterialParams_.setPe(1e4);
        coarseMaterialParams_.setPe(1e4);
        fineMaterialParams_.setLambda(2.0);
        coarseMaterialParams_.setLambda(2.0);
    }

    template<class ElementSolution>
    PermeabilityType permeability(const Element& element,
                                  const SubControlVolume& scv,
                                  const ElementSolution& elemSol) const
    {
        if (isFineMaterial_(scv.dofPosition()))
            return fineK_;
        else
            return coarseK_;
    }

    /*!
     * \brief Define the porosity \f$[-]\f$ of the soil
     *
     * \param element     The finite element
     * \param fvGeometry  The finite volume geometry
     * \param scvIdx      The local index of the sub-control volume where
     *                    the porosity needs to be defined
     */
    template<class ElementSolution>
    Scalar porosity(const Element &element,
                    const SubControlVolume &scv,
                    const ElementSolution &elemSol) const
    {
        return porosity_;
    }

    /*!
     * \brief Function for defining the parameters needed by constitutive relationships (kr-sw, pc-sw, etc.).
     *
     * \param pos The global position of the sub-control volume.
     * \return the material parameters object
     */
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    {
        if (isFineMaterial_(globalPos))
            return fineMaterialParams_;
        else
            return coarseMaterialParams_;
    }

private:
    /*!
     * \brief Returns whether a given global position is in the
     *        fine-permeability region or not.
     */
    static bool isFineMaterial_(const GlobalPosition &pos)
    {
        return
            30 - eps_ <= pos[0] && pos[0] <= 50 + eps_ &&
            20 - eps_ <= pos[1] && pos[1] <= 40 + eps_;
    }

    Scalar coarseK_;
    Scalar fineK_;
    Scalar porosity_;
    MaterialLawParams fineMaterialParams_;
    MaterialLawParams coarseMaterialParams_;
    static constexpr Scalar eps_ = 1e-6;
};

}

#endif
