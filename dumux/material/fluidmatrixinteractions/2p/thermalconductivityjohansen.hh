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
 *
 * \brief   Relation for the saturation-dependent effective thermal conductivity
 */
#ifndef THERMALCONDUCTIVITY_JOHANSEN_HH
#define THERMALCONDUCTIVITY_JOHANSEN_HH

#include <algorithm>
#include <dumux/common/basicproperties.hh>
#include <dumux/common/propertysystem.hh>

namespace Dumux
{

struct JohansenIndices
{
    static const int wPhaseIdx = 0;
    static const int nPhaseIdx = 1;
};

/*!
 * \ingroup fluidmatrixinteractionslaws
 *
 * \brief Relation for the saturation-dependent effective thermal conductivity
 *
 * The Johansen method (Johansen 1975) computes the thermal conductivity of dry and the
 * wet soil material and uses a root function of the wetting saturation to compute the
 * effective thermal conductivity for a two-phase fluidsystem. The individual thermal
 * conductivities are calculated as geometric mean of the thermal conductivity of the porous
 * material and of the respective fluid phase.
 * The material law is:
 * \f[
 \lambda_\text{eff} = \lambda_{\text{dry}} + \sqrt{(S_w)} \left(\lambda_\text{wet} - \lambda_\text{dry}\right)
 \f]
 *
 * with
 * \f[
 \lambda_\text{wet} = \lambda_{solid}^{\left(1-\phi\right)}*\lambda_w^\phi
 \f]
 * and the semi-empirical relation
 *
 * \f[
 \lambda_\text{dry} = \frac{0.135*\rho_s*\phi + 64.7}{\rho_s - 0.947 \rho_s*\phi}.
 \f]
 *
 * Source: Phdthesis (Johansen1975) Johansen, O. Thermal conductivity of soils Norw. Univ. of Sci. Technol., Trondheim, Norway, 1975
 */
template<class Scalar, class Indices = JohansenIndices>
class ThermalConductivityJohansen
{
public:
    /*!
     * \brief Returns the effective thermal conductivity \f$[W/(m K)]\f$ after Johansen (1975).
     *
     * \param volVars volume variables
     * \param spatialParams spatial parameters
     * \param element element (to be passed to spatialParams)
     * \param fvGeometry fvGeometry (to be passed to spatialParams)
     * \param scvIdx scvIdx (to be passed to spatialParams)
     *
     * \return Effective thermal conductivity \f$[W/(m K)]\f$ after Johansen (1975)
     *
     * This formulation is semi-empirical and fitted to quartz sand.
     * This gives an interpolation of the effective thermal conductivities of a porous medium
     * filled with the non-wetting phase and a porous medium filled with the wetting phase.
     * These two effective conductivities are computed as geometric mean of the solid and the
     * fluid conductivities and interpolated with the Kersten number.<br>
     * Johansen, O. 1975. Thermal conductivity of soils. Ph.D. diss. Norwegian Univ.
     *                    of Sci. and Technol., Trondheim. (Draft Transl. 637. 1977. U.S. Army
     *                    Corps of Eng., Cold Regions Res. and Eng. Lab., Hanover, NH.)
     */
    template<class VolumeVariables, class SpatialParams, class Element, class FVGeometry>
    static Scalar effectiveThermalConductivity(const VolumeVariables& volVars,
                                               const SpatialParams& spatialParams,
                                               const Element& element,
                                               const FVGeometry& fvGeometry,
                                               int scvIdx)
    {
        Scalar sw = volVars.saturation(Indices::wPhaseIdx);
        Scalar lambdaW = volVars.fluidThermalConductivity(Indices::wPhaseIdx);
        Scalar lambdaN = volVars.fluidThermalConductivity(Indices::nPhaseIdx);
        Scalar lambdaSolid = volVars.solidThermalConductivity();
        Scalar porosity = volVars.porosity();
        Scalar rhoSolid = spatialParams.solidDensity(element, fvGeometry, scvIdx);

        return effectiveThermalConductivity(sw, lambdaW, lambdaN, lambdaSolid, porosity, rhoSolid);
    }

    /*!
     * \brief Returns the effective thermal conductivity \f$[W/(m K)]\f$ after Johansen (1975).
     *
     * \param sw The saturation of the wetting phase
     * \param lambdaW the thermal conductivity of the wetting phase
     * \param lambdaN the thermal conductivity of the non-wetting phase
     * \param lambdaSolid the thermal conductivity of the solid phase
     * \param porosity The porosity
     *
     * \return Effective thermal conductivity \f$[W/(m K)]\f$ after Johansen (1975)
     */
    static Scalar effectiveThermalConductivity(const Scalar Sw,
                                               const Scalar lambdaW,
                                               const Scalar lambdaN,
                                               const Scalar lambdaSolid,
                                               const Scalar porosity,
                                               const Scalar rhoSolid)
    {
        const Scalar satW = std::max<Scalar>(0.0, Sw);

        const Scalar kappa = 15.6; // fitted to medium quartz sand
        const Scalar rhoBulk = rhoSolid*porosity;
        const Scalar lSat = std::pow(lambdaSolid, (1.0 - porosity)) * std::pow(lambdaW, porosity);
        const Scalar lDry = (0.135*rhoBulk + 64.7)/(rhoSolid - 0.947*rhoBulk);
        const Scalar Ke = (kappa*satW)/(1+(kappa-1)*satW);// Kersten number, equation 13

        return lDry + Ke * (lSat - lDry); // equation 14
    }
};
}
#endif