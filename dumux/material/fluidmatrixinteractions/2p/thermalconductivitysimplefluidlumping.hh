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
#ifndef THERMALCONDUCTIVITY_SIMPLE_FLUID_LUMPING_HH
#define THERMALCONDUCTIVITY_SIMPLE_FLUID_LUMPING_HH

#include <algorithm>

namespace Dumux
{
/*!
 * \ingroup fluidmatrixinteractionslaws
 *
 */
template<class TypeTag>
class ThermalConductivitySimpleFluidLumping
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    enum { numEnergyEquations = GET_PROP_VALUE(TypeTag, NumEnergyEquations)};


public:
    /*!
     * \brief Returns the effective thermal conductivity \f$[W/(m K)]\f$.
     *
     * \param sw The saturation of the wetting phase
     * \param lambdaW the thermal conductivity of the wetting phase
     * \param lambdaN the thermal conductivity of the non-wetting phase
     * \param lambdaSolid the thermal conductivity of the solid phase
     * \param porosity The porosity
     *
     * \return Effective thermal conductivity of the fluid phases
     */
    static Scalar effectiveThermalConductivity(const Scalar sw,
                                               const Scalar lambdaW,
                                               const Scalar lambdaN,
                                               const Scalar lambdaSolid,
                                               const Scalar porosity)
    {
    	assert(numEnergyEquations != 3) ;

        // Franz Lindner / Shi & Wang 2011
            const Scalar satW = std::max<Scalar>(0.0, sw);

            const Scalar kfeff = porosity *((1.-satW)*lambdaN + satW*lambdaW) ; // arithmetic

            Scalar keff ;

            if ( numEnergyEquations == 2){ // solid dealed with individually (extra balance equation)
            	keff = kfeff ;
            }
            else{
                const Scalar kseff = (1.0-porosity)  * lambdaSolid ;
                keff = kfeff  + kseff;
            }

            return keff ;
    }
};
}
#endif