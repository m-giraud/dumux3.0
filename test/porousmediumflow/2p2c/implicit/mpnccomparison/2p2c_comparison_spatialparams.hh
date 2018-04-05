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
 * \brief The spatial parameters for the 2p2c mpnc comparison problem
 */

#ifndef DUMUX_MPNC_COMPARISON_SPATIAL_PARAMS_HH
#define DUMUX_MPNC_COMPARISON_SPATIAL_PARAMS_HH

#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedlinearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>

namespace Dumux {

/*!
 * \ingroup MPNCTests
 * \brief The spatial parameters for the ObstacleProblem
 */
//forward declaration
template<class TypeTag>
class TwoPTwoCComparisonSpatialParams;

namespace Properties {

// The spatial parameters TypeTag
NEW_TYPE_TAG(TwoPTwoCComparisonSpatialParams);

// Set the spatial parameters
SET_TYPE_PROP(TwoPTwoCComparisonSpatialParams, SpatialParams, TwoPTwoCComparisonSpatialParams<TypeTag>);

// Set the material Law
SET_PROP(TwoPTwoCComparisonSpatialParams, MaterialLaw)
{
private:
    // define the material law which is parameterized by effective
    // saturations
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using EffectiveLaw = RegularizedBrooksCorey<Scalar>;
public:
    // define the material law parameterized by absolute saturations
    using type = EffToAbsLaw<EffectiveLaw>;
};
}

/**
 * \ingroup MPNCModel
 * \ingroup ImplicitTestProblems
 * \brief Definition of the spatial params properties for the obstacle problem
 *
 */
template<class TypeTag>
class TwoPTwoCComparisonSpatialParams : public FVSpatialParams<TypeTag>
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
    //! export permeability type
    using PermeabilityType = Scalar;
    //! export the type used for the material law
    using MaterialLaw = typename GET_PROP_TYPE(TypeTag, MaterialLaw);
    using MaterialLawParams = typename MaterialLaw::Params;


    TwoPTwoCComparisonSpatialParams(const Problem &problem) : ParentType(problem)
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
