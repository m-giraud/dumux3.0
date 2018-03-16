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
 *  \file
 * \ingroup TwoPNCModel
 * \brief Adaption of the fully implicit scheme to the
 *        two-phase n-component fully implicit model.
 *
 * This model implements two-phase n-component flow of two compressible and
 * partially miscible fluids \f$\alpha \in \{ w, n \}\f$ composed of the n components
 * \f$\kappa \in \{ w, n,\cdots \}\f$ in combination with mineral precipitation and dissolution.
 * The solid phases. The standard multiphase Darcy
 * approach is used as the equation for the conservation of momentum:
 * \f[
 v_\alpha = - \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K}
 \left(\text{grad}\, p_\alpha - \varrho_{\alpha} \mbox{\bf g} \right)
 * \f]
 *
 * By inserting this into the equations for the conservation of the
 * components, one gets one transport equation for each component
 * \f{eqnarray}
 && \frac{\partial (\sum_\alpha \varrho_\alpha X_\alpha^\kappa \phi S_\alpha )}
 {\partial t}
 - \sum_\alpha  \text{div} \left\{ \varrho_\alpha X_\alpha^\kappa
 \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K}
 (\text{grad}\, p_\alpha - \varrho_{\alpha}  \mbox{\bf g}) \right\}
 \nonumber \\ \nonumber \\
    &-& \sum_\alpha \text{div} \left\{{\bf D_{\alpha, pm}^\kappa} \varrho_{\alpha} \text{grad}\, X^\kappa_{\alpha} \right\}
 - \sum_\alpha q_\alpha^\kappa = 0 \qquad \kappa \in \{w, a,\cdots \} \, ,
 \alpha \in \{w, g\}
 \f}
 *
 * The solid or mineral phases are assumed to consist of a single component.
 * Their mass balance consist only of a storage and a source term:
 *  \f$\frac{\partial \varrho_\lambda \phi_\lambda )} {\partial t}
 *  = q_\lambda\f$
 *
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as
 * spatial and the implicit Euler method as time discretization.
 *
 * By using constitutive relations for the capillary pressure \f$p_c =
 * p_n - p_w\f$ and relative permeability \f$k_{r\alpha}\f$ and taking
 * advantage of the fact that \f$S_w + S_n = 1\f$ and \f$X^\kappa_w + X^\kappa_n = 1\f$, the number of
 * unknowns can be reduced to number of components.
 *
 * The used primary variables are, like in the two-phase model, either \f$p_w\f$ and \f$S_n\f$
 * or \f$p_n\f$ and \f$S_w\f$. The formulation which ought to be used can be
 * specified by setting the <tt>Formulation</tt> property to either
 * TwoPTwoCIndices::pwsn or TwoPTwoCIndices::pnsw. By
 * default, the model uses \f$p_w\f$ and \f$S_n\f$.
 *
 * Moreover, the second primary variable depends on the phase state, since a
 * primary variable switch is included. The phase state is stored for all nodes
 * of the system. The model is uses mole fractions.
 *Following cases can be distinguished:
 * <ul>
 *  <li> Both phases are present: The saturation is used (either \f$S_n\f$ or \f$S_w\f$, dependent on the chosen <tt>Formulation</tt>),
 *      as long as \f$ 0 < S_\alpha < 1\f$</li>.
 *  <li> Only wetting phase is present: The mole fraction of, e.g., air in the wetting phase \f$x^a_w\f$ is used,
 *      as long as the maximum mole fraction is not exceeded (\f$x^a_w<x^a_{w,max}\f$)</li>
 *  <li> Only non-wetting phase is present: The mole fraction of, e.g., water in the non-wetting phase, \f$x^w_n\f$, is used,
 *      as long as the maximum mole fraction is not exceeded (\f$x^w_n<x^w_{n,max}\f$)</li>
 * </ul>
 *
 * For the other components, the mole fraction \f$x^\kappa_w\f$ is the primary variable.
 */

#ifndef DUMUX_2PNC_MODEL_HH
#define DUMUX_2PNC_MODEL_HH

#include <dune/common/fvector.hh>

#include <dumux/common/properties.hh>

#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidmatrixinteractions/diffusivitymillingtonquirk.hh>
#include <dumux/material/fluidmatrixinteractions/2p/thermalconductivitysomerton.hh>

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/porousmediumflow/compositional/localresidual.hh>
#include <dumux/porousmediumflow/compositional/switchableprimaryvariables.hh>
#include <dumux/porousmediumflow/nonisothermal/model.hh>
#include <dumux/porousmediumflow/nonisothermal/indices.hh>
#include <dumux/porousmediumflow/nonisothermal/vtkoutputfields.hh>

#include "indices.hh"
#include "volumevariables.hh"
#include "primaryvariableswitch.hh"
#include "vtkoutputfields.hh"

namespace Dumux
{

/*!
 * \ingroup TwoPNCModel
 * \brief Specifies a number properties of two-phase n-component models.
 *
 * \ţparam nComp the number of components to be considered.
 */
template<int nComp>
struct TwoPNCModelTraits
{
    static constexpr int numEq() { return nComp; }
    static constexpr int numPhases() { return 2; }
    static constexpr int numComponents() { return nComp; }
    static constexpr int numMajorComponents() { return 2; }

    static constexpr bool enableAdvection() { return true; }
    static constexpr bool enableMolecularDiffusion() { return true; }
    static constexpr bool enableEnergyBalance() { return false; }
};

namespace Properties
{
//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////
NEW_TYPE_TAG(TwoPNC, INHERITS_FROM(PorousMediumFlow));
NEW_TYPE_TAG(TwoPNCNI, INHERITS_FROM(TwoPNC));

//////////////////////////////////////////////////////////////////
// Properties for the isothermal 2pnc model
//////////////////////////////////////////////////////////////////
//! The primary variables vector for the 2pnc model
SET_PROP(TwoPNC, PrimaryVariables)
{
private:
    using PrimaryVariablesVector = Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                                                     GET_PROP_TYPE(TypeTag, ModelTraits)::numEq()>;
public:
    using type = SwitchablePrimaryVariables<PrimaryVariablesVector, int>;
};

SET_TYPE_PROP(TwoPNC, PrimaryVariableSwitch, TwoPNCPrimaryVariableSwitch<TypeTag>);         //!< The primary variable switch for the 2pnc model
SET_TYPE_PROP(TwoPNC, VolumeVariables, TwoPNCVolumeVariables<TypeTag>);                     //!< the VolumeVariables property
SET_TYPE_PROP(TwoPNC, SpatialParams, FVSpatialParams<TypeTag>);                             //!< Use the FVSpatialParams by default

//! Set the model traits
SET_PROP(TwoPNC, ModelTraits)
{
private:
    //! we use the number of components specified by the fluid system here
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    static_assert(FluidSystem::numPhases == 2, "Only fluid systems with 2 fluid phases are supported by the 2p-nc model!");
public:
    using type = TwoPNCModelTraits<FluidSystem::numComponents>;
};

//! Set the vtk output fields specific to this model
SET_PROP(TwoPNC, VtkOutputFields)
{
private:
   using FluidSystem =  typename GET_PROP_TYPE(TypeTag, FluidSystem);
   using Indices = typename GET_PROP_TYPE(TypeTag, Indices);

public:
    using type = TwoPNCVtkOutputFields<FluidSystem, Indices>;
};

SET_TYPE_PROP(TwoPNC, LocalResidual, CompositionalLocalResidual<TypeTag>);                  //!< Use the compositional local residual

SET_INT_PROP(TwoPNC, ReplaceCompEqIdx, GET_PROP_TYPE(TypeTag, FluidSystem)::numComponents); //!< Per default, no component mass balance is replaced
SET_INT_PROP(TwoPNC, Formulation, TwoPNCFormulation::pwsn);                                 //!< Default formulation is pw-Sn, overwrite if necessary

SET_BOOL_PROP(TwoPNC, SetMoleFractionsForWettingPhase, true);  //!< Set the primary variables mole fractions for the wetting or non-wetting phase
SET_BOOL_PROP(TwoPNC, UseMoles, true);                         //!< Use mole fractions in the balance equations by default

//! The indices required by the isothermal 2pnc model
SET_PROP(TwoPNC, Indices)
{
private:
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
public:
    using type = TwoPNCIndices<FluidSystem, /*PVOffset=*/0>;
};

//! Use the model after Millington (1961) for the effective diffusivity
SET_TYPE_PROP(TwoPNC, EffectiveDiffusivityModel, DiffusivityMillingtonQuirk<typename GET_PROP_TYPE(TypeTag, Scalar)>);

//! This model uses the compositional fluid state
SET_PROP(TwoPNC, FluidState)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
public:
    using type = CompositionalFluidState<Scalar, FluidSystem>;
};

/////////////////////////////////////////////////
// Properties for the non-isothermal 2pnc model
/////////////////////////////////////////////////

//! Set the non-isothermal model traits
SET_PROP(TwoPNCNI, ModelTraits)
{
private:
    //! we use the number of components specified by the fluid system here
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    static_assert(FluidSystem::numPhases == 2, "Only fluid systems with 2 fluid phases are supported by the 2p-nc model!");
    using IsothermalTraits = TwoPNCModelTraits<FluidSystem::numComponents>;
public:
    using type = PorousMediumFlowNIModelTraits<IsothermalTraits>;
};

//! Set non-isothermal output fields
SET_PROP(TwoPNCNI, VtkOutputFields)
{
private:
    using FluidSystem =  typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using IsothermalFields = TwoPNCVtkOutputFields<FluidSystem, Indices>;
public:
    using type = EnergyVtkOutputFields<IsothermalFields>;
};

//! set non-isothermal Indices
SET_PROP(TwoPNCNI, Indices)
{
private:
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using IsothermalIndices = TwoPNCIndices<FluidSystem, /*PVOffset=*/0>;
    static constexpr int numEq = GET_PROP_TYPE(TypeTag, ModelTraits)::numEq();
public:
    using type = EnergyIndices<IsothermalIndices, numEq, 0>;
};


//! Somerton is used as default model to compute the effective thermal heat conductivity
SET_PROP(TwoPNCNI, ThermalConductivityModel)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
public:
    using type = ThermalConductivitySomerton<Scalar, Indices>;
};

} // end namespace Properties
} // end namespace Dumux

#endif
