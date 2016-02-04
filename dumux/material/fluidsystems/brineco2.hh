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
 * \brief A fluid system with water and gas as phases and brine and CO2
 *        as components.
 */
#ifndef DUMUX_BRINE_CO2_SYSTEM_HH
#define DUMUX_BRINE_CO2_SYSTEM_HH

#include <dumux/material/idealgas.hh>

#include <dumux/common/propertysystem.hh>
#include <dumux/common/basicproperties.hh>
#include <dumux/material/fluidsystems/base.hh>

#include <dumux/material/fluidsystems/defaultcomponents.hh>
#include <dumux/material/components/co2.hh>
#include <dumux/material/components/co2tablereader.hh>
#include <dumux/material/components/tabulatedcomponent.hh>

#include <dumux/material/binarycoefficients/brine_co2.hh>

namespace Dumux
{
#include <dumux/material/components/co2tables.inc>

namespace FluidSystems{
/*!
 * \ingroup Fluidsystems
 * \brief A compositional fluid with brine and carbon as
 *        components in both, the liquid and the gas (supercritical) phase.
 *
 * This class provides acess to the Brine CO2 fluid system when no property system is used.
 * For Dumux users, using BrineCO2FluidSystem<TypeTag> and the documentation therein is
 * recommended.
 *
 *  The user can provide their own material table for co2 properties.
 *  This fluidsystem is initialized as default with the tabulated version of
 *  water of the IAPWS-formulation, and the tabularized adapter to transfer
 *  this into brine.
 *  In the non-TypeTagged version, salinity information has to be provided with
 *  the init() methods.
 */
template<class Scalar,
         class CO2Table,
         class H2Otype = Dumux::TabulatedComponent<Scalar, Dumux::H2O<Scalar> >,
         class BrineRawComponent = Dumux::Brine<Scalar, Dumux::H2O<Scalar> >,
         class Brinetype = Dumux::TabulatedComponent<Scalar, BrineRawComponent> >
class BrineCO2
: public BaseFluidSystem<Scalar, BrineCO2<Scalar, CO2Table, H2Otype, BrineRawComponent, Brinetype> >
{
    typedef BrineCO2<Scalar, CO2Table, H2Otype, BrineRawComponent, Brinetype> ThisType;
    typedef BaseFluidSystem <Scalar, ThisType> Base;


    typedef Dumux::BinaryCoeff::Brine_CO2<Scalar, CO2Table> Brine_CO2;

public:
    typedef Dumux::NullParameterCache ParameterCache;
    typedef H2Otype H2O;
//    typedef Dumux::Brine<Scalar, H2O> BrineRawComponent; //salinity is stored into the raw brine component, wether tabulated or not.
    typedef Brinetype Brine;
    typedef typename Dumux::CO2<Scalar, CO2Table> CO2;

    static const int numComponents = 2;
    static const int numPhases = 2;

    static const int lPhaseIdx = 0; // index of the liquid phase
    static const int gPhaseIdx = 1; // index of the gas phase
    static const int wPhaseIdx = lPhaseIdx;
    static const int nPhaseIdx = gPhaseIdx;
    static const int wCompIdx = 0;
    static const int nCompIdx = 1;
    static const int lCompIdx = wCompIdx;
    static const int gCompIdx = nCompIdx;

    static const int BrineIdx = 0;
    static const int CO2Idx = 1;

    /*!
     * \brief Return the human readable name of a fluid phase
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static const char *phaseName(int phaseIdx)
    {
        static const char *name[] = {
            "l",
            "g"
        };

        assert(0 <= phaseIdx && phaseIdx < numPhases);
        return name[phaseIdx];
    }

    /*!
     * \brief Return whether a phase is liquid
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static bool isLiquid(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        return phaseIdx != gPhaseIdx;
    }

    /*!
     * \brief Returns true if and only if a fluid phase is assumed to
     *        be an ideal mixture.
     *
     * We define an ideal mixture as a fluid phase where the fugacity
     * coefficients of all components times the pressure of the phase
     * are indepent on the fluid composition. This assumtion is true
     * if Henry's law and Rault's law apply. If you are unsure what
     * this function should return, it is safe to return false. The
     * only damage done will be (slightly) increased computation times
     * in some cases.
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static bool isIdealMixture(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        return true;
    }

    /*!
     * \brief Returns true if and only if a fluid phase is assumed to
     *        be compressible.
     *
     * Compressible means that the partial derivative of the density
     * to the fluid pressure is always larger than zero.
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static bool isCompressible(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        return true;
    }

    /*!
     * \brief Return the human readable name of a component
     *
     * \param compIdx The index of the component to consider
     */
    static const char *componentName(int compIdx)
    {
        static const char *name[] = {
            Brine::name(),
            CO2::name(),
        };

        assert(0 <= compIdx && compIdx < numComponents);
        return name[compIdx];
    }

    /*!
     * \brief Return the molar mass of a component in \f$\mathrm{[kg/mol]}\f$.
     *
     * \param compIdx The index of the component to consider
     */
    static Scalar molarMass(int compIdx)
    {
        static const Scalar M[] = {
            Brine::molarMass(),
            CO2::molarMass(),
        };

        assert(0 <= compIdx && compIdx < numComponents);
        return M[compIdx];
    }

    /****************************************
     * thermodynamic relations
     ****************************************/

    static void init(Scalar salinity)
    {
        init(/*startTemp=*/273.15, /*endTemp=*/623.15, /*tempSteps=*/100,
             /*startPressure=*/1e4, /*endPressure=*/40e6, /*pressureSteps=*/200, salinity);
    }

    static void init(Scalar startTemp, Scalar endTemp, int tempSteps,
                     Scalar startPressure, Scalar endPressure, int pressureSteps,
                     Scalar salinity)
    {
        if(H2O::isTabulated)
        {
            std::cout << "Initializing tables for the pure-water properties.\n";
            H2O::init(startTemp, endTemp, tempSteps,
                                startPressure, endPressure, pressureSteps);
        }
        // set the salinity of brine
        BrineRawComponent::salinity = salinity;

        if(Brine::isTabulated)
        {
            std::cout << "Initializing tables for the brine fluid properties.\n";
            Brine::init(startTemp, endTemp, tempSteps,
                                  startPressure, endPressure, pressureSteps);
        }
    }

    /*!
     * \brief Given a phase's composition, temperature, pressure, and
     *        the partial pressures of all components, return its
     *        density \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param fluidState The fluid state
     * \param phaseIdx The index of the phase
     */
    using Base::density;
    template <class FluidState>
    static Scalar density(const FluidState &fluidState,
                          int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);

        if (phaseIdx == lPhaseIdx) {
            // use normalized composition for to calculate the density
            // (the relations don't seem to take non-normalized
            // compositions too well...)
            Scalar xlBrine = std::min(1.0, std::max(0.0, fluidState.moleFraction(lPhaseIdx, BrineIdx)));
            Scalar xlCO2 = std::min(1.0, std::max(0.0, fluidState.moleFraction(lPhaseIdx, CO2Idx)));
            Scalar sumx = xlBrine + xlCO2;
            xlBrine /= sumx;
            xlCO2 /= sumx;

            Scalar result = liquidDensity_(temperature,
                                           pressure,
                                           xlBrine,
                                           xlCO2);

            Valgrind::CheckDefined(result);
            return result;
        }
        else {
            assert(phaseIdx == gPhaseIdx);

            // use normalized composition for to calculate the density
            // (the relations don't seem to take non-normalized
            // compositions too well...)
            Scalar xgBrine = std::min(1.0, std::max(0.0, fluidState.moleFraction(gPhaseIdx, BrineIdx)));
            Scalar xgCO2 = std::min(1.0, std::max(0.0, fluidState.moleFraction(gPhaseIdx, CO2Idx)));
            Scalar sumx = xgBrine + xgCO2;
            xgBrine /= sumx;
            xgCO2 /= sumx;

            Scalar result = gasDensity_(temperature,
                                        pressure,
                                        xgBrine,
                                        xgCO2);
            Valgrind::CheckDefined(result);
            return result;
        }
    }

    /*!
     * \brief Calculate the dynamic viscosity of a fluid phase \f$\mathrm{[Pa*s]}\f$
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     */
    using Base::viscosity;
    template <class FluidState>
    static Scalar viscosity(const FluidState &fluidState,
                            int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);
        Scalar result = 0;

        if (phaseIdx == lPhaseIdx) {
            // assume pure brine for the liquid phase. TODO: viscosity
            // of mixture
            result = Brine::liquidViscosity(temperature, pressure);
            }
        else
            result = CO2::gasViscosity(temperature, pressure);

        Valgrind::CheckDefined(result);
        return result;
    }

    /*!
     * \brief Returns the fugacity coefficient \f$\mathrm{[-]}\f$ of a component in a
     *        phase.
     *
     * The fugacity coefficient \f$\mathrm{\phi^\kappa_\alpha}\f$ of
     * component \f$\mathrm{\kappa}\f$ in phase \f$\mathrm{\alpha}\f$ is connected to
     * the fugacity \f$\mathrm{f^\kappa_\alpha}\f$ and the component's mole
     * fraction \f$\mathrm{x^\kappa_\alpha}\f$ by means of the relation
     *
     * \f[
     f^\kappa_\alpha = \phi^\kappa_\alpha\;x^\kappa_\alpha\;p_\alpha
     \f]
     * where \f$\mathrm{p_\alpha}\f$ is the pressure of the fluid phase.
     *
     * The fugacity itself is just an other way to express the
     * chemical potential \f$\mathrm{\zeta^\kappa_\alpha}\f$ of the component:
     *
     * \f[
     f^\kappa_\alpha := \exp\left\{\frac{\zeta^\kappa_\alpha}{k_B T_\alpha} \right\}
     \f]
     * where \f$\mathrm{k_B}\f$ is Boltzmann's constant.
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     * \param compIdx The index of the component
     */
    using Base::fugacityCoefficient;
    template <class FluidState>
    static Scalar fugacityCoefficient(const FluidState &fluidState,
                                      int phaseIdx,
                                      int compIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);
        assert(0 <= compIdx && compIdx < numComponents);

        if (phaseIdx == gPhaseIdx)
            // use the fugacity coefficients of an ideal gas. the
            // actual value of the fugacity is not relevant, as long
            // as the relative fluid compositions are observed,
            return 1.0;

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);
        assert(temperature > 0);
        assert(pressure > 0);

        // calulate the equilibrium composition for the given
        // temperature and pressure. TODO: calculateMoleFractions()
        // could use some cleanup.
        Scalar xlH2O, xgH2O;
        Scalar xlCO2, xgCO2;
        Brine_CO2::calculateMoleFractions(temperature,
                                          pressure,
                                          BrineRawComponent::salinity,
                                          /*knownPhaseIdx=*/-1,
                                          xlCO2,
                                          xgH2O);

        // normalize the phase compositions
        xlCO2 = std::max(0.0, std::min(1.0, xlCO2));
        xgH2O = std::max(0.0, std::min(1.0, xgH2O));

        xlH2O = 1.0 - xlCO2;
        xgCO2 = 1.0 - xgH2O;

        if (compIdx == BrineIdx) {
            Scalar phigH2O = 1.0;
            return phigH2O * xgH2O / xlH2O;
        }

        assert(compIdx == CO2Idx);

        Scalar phigCO2 = 1.0;
        return phigCO2 * xgCO2 / xlCO2;
    }

    /*!
     * \brief Returns the equilibrium concentration of the dissolved component
     *        in a phase.
     * \param fluidState An arbitrary fluid state
     * \param paramCache Parameter cache
     * \param phaseIdx The index of the fluid phase to consider
     */

    template <class FluidState>
    static Scalar equilibriumMoleFraction(const FluidState &fluidState,
                                      const ParameterCache &paramCache,
                                      int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);
        assert(temperature > 0);
        assert(pressure > 0);

        Scalar xgH2O;
        Scalar xlCO2;

        // calulate the equilibrium composition for the given
        // temperature and pressure.
        Brine_CO2::calculateMoleFractions(temperature,
                                                  pressure,
                                                  BrineRawComponent::salinity,
                                                  /*knownPhaseIdx=*/-1,
                                                  xlCO2,
                                                  xgH2O);

        if(phaseIdx == gPhaseIdx)
        {
            return xgH2O;
        }
        else
        {
            return xlCO2;
        }
    }


    /*!
     * \brief Calculate the molecular diffusion coefficient for a
     *        component in a fluid phase \f$\mathrm{[mol^2 * s / (kg*m^3)]}\f$
     *
     * Molecular diffusion of a compoent \f$\mathrm{\kappa}\f$ is caused by a
     * gradient of the chemical potential and follows the law
     *
     * \f[ J = - D \textbf{grad} mu_\kappa \f]
     *
     * where \f$\mathrm{\mu_\kappa}\f$ is the component's chemical potential,
     * \f$D\f$ is the diffusion coefficient and \f$\mathrm{J}\f$ is the
     * diffusive flux. \f$\mathrm{mu_\kappa}\f$ is connected to the component's
     * fugacity \f$\mathrm{f_\kappa}\f$ by the relation
     *
     * \f[ \mu_\kappa = R T_\alpha \mathrm{ln} \frac{f_\kappa}{p_\alpha} \f]
     *
     * where \f$\mathrm{p_\alpha}\f$ and \f$\mathrm{T_\alpha}\f$ are the fluid phase'
     * pressure and temperature.
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     * \param compIdx The index of the component to consider
     */
    using Base::diffusionCoefficient;
    template <class FluidState>
    static Scalar diffusionCoefficient(const FluidState &fluidState,
                                       int phaseIdx,
                                       int compIdx)
    {
        // TODO!
        DUNE_THROW(Dune::NotImplemented, "Diffusion coefficients");
    }

    /*!
     * \brief Given the phase compositions, return the binary
     *        diffusion coefficent \f$\mathrm{[m^2/s]}\f$ of two components in a phase.
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     * \param compIIdx Index of the component i
     * \param compJIdx Index of the component j
     */
    using Base::binaryDiffusionCoefficient;
    template <class FluidState>
    static Scalar binaryDiffusionCoefficient(const FluidState &fluidState,
                                             int phaseIdx,
                                             int compIIdx,
                                             int compJIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);
        assert(0 <= compIIdx && compIIdx < numComponents);
        assert(0 <= compJIdx && compJIdx < numComponents);

        if (compIIdx > compJIdx)
            std::swap(compIIdx, compJIdx);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);
        if (phaseIdx == lPhaseIdx) {
            assert(compIIdx == BrineIdx);
            assert(compJIdx == CO2Idx);

            Scalar result = Brine_CO2::liquidDiffCoeff(temperature, pressure);
            Valgrind::CheckDefined(result);
            return result;
        }
        else {
            assert(phaseIdx == gPhaseIdx);
            assert(compIIdx == BrineIdx);
            assert(compJIdx == CO2Idx);

            Scalar result = Brine_CO2::gasDiffCoeff(temperature, pressure);
            Valgrind::CheckDefined(result);
            return result;
        }
    }

    /*!
     * \brief Given the phase composition, return the specific
     *        phase enthalpy \f$\mathrm{[J/kg]}\f$.
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     */
    using Base::enthalpy;
    template <class FluidState>
    static Scalar enthalpy(const FluidState &fluidState,
                           int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);

        if (phaseIdx == lPhaseIdx) {
            Scalar XlCO2 = fluidState.massFraction(phaseIdx, CO2Idx);

            Scalar result = liquidEnthalpyBrineCO2_(temperature,
                                                    pressure,
                                                    BrineRawComponent::salinity,
                                                    XlCO2);
            Valgrind::CheckDefined(result);
            return result;
        }
        else {
            Scalar result = 0;
            result +=
                Brine::gasEnthalpy(temperature, pressure) *
                fluidState.massFraction(gPhaseIdx, BrineIdx);
            result +=
                CO2::gasEnthalpy(temperature, pressure) *
                fluidState.massFraction(gPhaseIdx, CO2Idx);
            Valgrind::CheckDefined(result);
            return result;
        }
    }

    /*!
     * \brief Thermal conductivity of a fluid phase \f$\mathrm{[W/(m K)]}\f$.
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     */
    using Base::thermalConductivity;
    template <class FluidState>
    static Scalar thermalConductivity(const FluidState &fluidState,
                                      int phaseIdx)
    {
        // TODO way too simple!
        if (phaseIdx == lPhaseIdx)
            return  0.6; // conductivity of water[W / (m K ) ]

        // gas phase
        return 0.025; // conductivity of air [W / (m K ) ]
    }

    /*!
     * \copydoc BaseFluidSystem::heatCapacity
     *
     * We employ the heat capacity of the pure phases.
     * Todo: Include compositional effects.
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     */
    using Base::heatCapacity;
    template <class FluidState>
    static Scalar heatCapacity(const FluidState &fluidState,
                               int phaseIdx)
    {
        if(phaseIdx == wPhaseIdx)
            return H2O::liquidHeatCapacity(fluidState.temperature(phaseIdx),
                                           fluidState.pressure(phaseIdx));
        else
            return CO2::liquidHeatCapacity(fluidState.temperature(phaseIdx),
                                       fluidState.pressure(phaseIdx));
    }

private:
    static Scalar gasDensity_(Scalar T,
                              Scalar pg,
                              Scalar xgH2O,
                              Scalar xgCO2)
    {
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(pg);
        Valgrind::CheckDefined(xgH2O);
        Valgrind::CheckDefined(xgCO2);

        Scalar gasDensity = CO2::gasDensity(T, pg);
        return gasDensity;
    }

    /***********************************************************************/
    /*                                                                     */
    /* Total brine density with dissolved CO2                              */
    /* rho_{b,CO2} = rho_w + contribution(salt) + contribution(CO2)        */
    /*                                                                     */
    /***********************************************************************/
    static Scalar liquidDensity_(Scalar T,
                                 Scalar pl,
                                 Scalar xlH2O,
                                 Scalar xlCO2)
    {
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(pl);
        Valgrind::CheckDefined(xlH2O);
        Valgrind::CheckDefined(xlCO2);

        if(T < 273.15) {
            DUNE_THROW(NumericalProblem,
                       "Liquid density for Brine and CO2 is only "
                       "defined above 273.15K (is" << T << ")");
        }
        if(pl >= 2.5e8) {
            DUNE_THROW(NumericalProblem,
                       "Liquid density for Brine and CO2 is only "
                       "defined below 250MPa (is" << pl << ")");
        }

        Scalar rho_brine = Brine::liquidDensity(T, pl);
        Scalar rho_pure = H2O::liquidDensity(T, pl);
        Scalar rho_lCO2 = liquidDensityWaterCO2_(T, pl, xlH2O, xlCO2);
        Scalar contribCO2 = rho_lCO2 - rho_pure;

        return rho_brine + contribCO2;
    }

    static Scalar liquidDensityWaterCO2_(Scalar temperature,
                                         Scalar pl,
                                         Scalar xlH2O,
                                         Scalar xlCO2)
    {
        const Scalar M_CO2 = CO2::molarMass();
        const Scalar M_H2O = H2O::molarMass();

        const Scalar tempC = temperature - 273.15;        /* tempC : temperature in °C */
        const Scalar rho_pure = H2O::liquidDensity(temperature, pl);
        xlH2O = 1.0 - xlCO2; // xlH2O is available, but in case of a pure gas phase
                             // the value of M_T for the virtual liquid phase can become very large
        const Scalar M_T = M_H2O * xlH2O + M_CO2 * xlCO2;
        const Scalar V_phi =
            (37.51 +
             tempC*(-9.585e-2 +
                    tempC*(8.74e-4 -
                           tempC*5.044e-7))) / 1.0e6;
        return 1/ (xlCO2 * V_phi/M_T + M_H2O * xlH2O / (rho_pure * M_T));
    }

    static Scalar liquidEnthalpyBrineCO2_(Scalar T,
                                          Scalar p,
                                          Scalar S,
                                          Scalar X_CO2_w)
    {
        /* X_CO2_w : mass fraction of CO2 in brine */

        /* same function as enthalpy_brine, only extended by CO2 content */

        /*Numerical coefficents from PALLISER*/
        static const Scalar f[] = {
            2.63500E-1, 7.48368E-6, 1.44611E-6, -3.80860E-10
        };

        /*Numerical coefficents from MICHAELIDES for the enthalpy of brine*/
        static const Scalar a[4][3] = {
            { 9633.6, -4080.0, +286.49 },
            { +166.58, +68.577, -4.6856 },
            { -0.90963, -0.36524, +0.249667E-1 },
            { +0.17965E-2, +0.71924E-3, -0.4900E-4 }
        };

        Scalar theta, h_NaCl;
        Scalar m, h_ls, h_ls1, d_h;
        Scalar S_lSAT, delta_h;
        int i, j;
        Scalar delta_hCO2, hg, hw;

        theta = T - 273.15;

        S_lSAT = f[0] + f[1]*theta + f[2]*theta*theta + f[3]*theta*theta*theta;
        /*Regularization*/
        if (S>S_lSAT) {
            S = S_lSAT;
        }

        hw = H2O::liquidEnthalpy(T, p) /1E3; /* kJ/kg */

        /*DAUBERT and DANNER*/
        /*U=*/h_NaCl = (3.6710E4*T + 0.5*(6.2770E1)*T*T - ((6.6670E-2)/3)*T*T*T
                        +((2.8000E-5)/4)*(T*T*T*T))/(58.44E3)- 2.045698e+02; /* kJ/kg */

        m = (1E3/58.44)*(S/(1-S));
        i = 0;
        j = 0;
        d_h = 0;

        for (i = 0; i<=3; i++) {
            for (j=0; j<=2; j++) {
                d_h = d_h + a[i][j] * pow(theta, i) * pow(m, j);
            }
        }
        /* heat of dissolution for halite according to Michaelides 1971 */
        delta_h = (4.184/(1E3 + (58.44 * m)))*d_h;

        /* Enthalpy of brine without CO2 */
        h_ls1 =(1-S)*hw + S*h_NaCl + S*delta_h; /* kJ/kg */

        /* heat of dissolution for CO2 according to Fig. 6 in Duan and Sun 2003. (kJ/kg)
           In the relevant temperature ranges CO2 dissolution is
           exothermal */
        delta_hCO2 = (-57.4375 + T * 0.1325) * 1000/44;

        /* enthalpy contribution of CO2 (kJ/kg) */
        hg = CO2::liquidEnthalpy(T, p)/1E3 + delta_hCO2;

        /* Enthalpy of brine with dissolved CO2 */
        h_ls = (h_ls1 - X_CO2_w*hw + hg*X_CO2_w)*1E3; /*J/kg*/

        return (h_ls);
    }
};
} // end namespace FluidSystems


#ifdef DUMUX_PROPERTIES_HH
// forward defintions of the property tags
namespace Properties
{
NEW_PROP_TAG(Scalar);
NEW_PROP_TAG(CO2Table);
NEW_PROP_TAG(ProblemSalinity);
// Set Co2 tables
SET_TYPE_PROP(NumericModel, CO2Table, Dumux::CO2Tables);
// Set salinity defaults
SET_SCALAR_PROP(NumericModel, ProblemSalinity, 1e-3);
}

/*!
 * \brief A compositional fluid with brine and carbon as
 *        components in both, the liquid and the gas (supercritical) phase.
 *
 *  This fluidsystem is initialized as default with the tabulated version of
 *  water of the IAPWS-formulation, and the tabularized adapter to transfer
 *  this into brine.
 *  To change the component formulation (e.g. change tabularization to avoid
 *  init routine), change the default components via the property "Components":
 *
 *
     \verbatim
        // Select other compoenents
        SET_PROP(myApplicationProperty, Components) : public GET_PROP(TypeTag, DefaultComponents)
        {
            typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
                // Do not use the defaults that are the following
                //    typedef Dumux::TabulatedComponent<Scalar, Dumux::H2O<Scalar> > H2O;
                //    typedef Dumux::Brine<Scalar, Dumux::H2O<Scalar> >  BrineRawComponent;
                //    typedef Dumux::TabulatedComponent<Scalar,BrineRawComponent > Brine;

            // Apply the following component classes:
            typedef Dumux::H2O<Scalar> H2O;
            typedef Dumux::Brine<Scalar, H2O> BrineRawComponent;
            typedef typename BrineRawComponent Brine;// all components have to be redefined,
                                                     // the applied H2O and Brine implemementations.
        };
    \endverbatim.
     Also remember to initialize all tabulated components (FluidSystem::init()), while this
     is not necessary for non-tabularized ones.
 *
 *   The desired material tables for CO2 can be defined via
 *      SET_TYPE_PROP(myApplicationProperty, CO2Table, myCO2Tables);
 *   or use the default tables. Do not forget to include the tables.
 *   Salinity is specified via the appropriate property.
 */

template <class TypeTag, bool verbose=true>
class BrineCO2FluidSystem
: public FluidSystems::BrineCO2<typename GET_PROP_TYPE(TypeTag, Scalar),
                                typename GET_PROP_TYPE(TypeTag, CO2Table),
                                typename GET_PROP(TypeTag, Components)::H2O,
                                typename GET_PROP(TypeTag, Components)::BrineRawComponent,
                                typename GET_PROP(TypeTag, Components)::Brine>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename FluidSystems::BrineCO2<typename GET_PROP_TYPE(TypeTag, Scalar),
            typename GET_PROP_TYPE(TypeTag, CO2Table),
            typename GET_PROP(TypeTag, Components)::H2O,
            typename GET_PROP(TypeTag, Components)::BrineRawComponent,
            typename GET_PROP(TypeTag, Components)::Brine> ParentType;

public:
    static void init()
    {
        ParentType::init(GET_PARAM_FROM_GROUP(TypeTag, Scalar, Problem, Salinity));
    }
    static void init(Scalar startTemp, Scalar endTemp, int tempSteps,
                     Scalar startPressure, Scalar endPressure, int pressureSteps)
    {
        ParentType::init(startTemp, endTemp, tempSteps,
                startPressure, endPressure, pressureSteps, GET_PARAM_FROM_GROUP(TypeTag, Scalar, Problem, Salinity));
    }
};
#endif
} // end namespace Dumux

#endif