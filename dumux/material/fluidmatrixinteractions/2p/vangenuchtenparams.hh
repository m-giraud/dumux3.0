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
 * \brief Specification of the material parameters
 *       for the van Genuchten constitutive relations.
 */
#ifndef VAN_GENUCHTEN_PARAMS_HH
#define VAN_GENUCHTEN_PARAMS_HH

namespace Dumux
{
/*!
 *
 * \brief Specification of the material parameters
 *       for the van Genuchten constitutive relations.
 *
 *       In this implementation setting either the \f$n\f$ or \f$m\f$ shape parameter
 *       automatically calculates the other. I.e. they cannot be set independently.
 *
 * \ingroup fluidmatrixinteractionsparams
 */
template<class ScalarT>
class VanGenuchtenParams
{
public:
    typedef ScalarT Scalar;

    VanGenuchtenParams()
    {}

    VanGenuchtenParams(Scalar vgAlpha, Scalar vgn)
    {
        setVgAlpha(vgAlpha);
        setVgn(vgn);
    };

    /*!
     * \brief Return the \f$\alpha\f$ shape parameter [1/Pa] of van Genuchten's
     *        curve.
     */
    Scalar vgAlpha() const
    { return vgAlpha_; }

    /*!
     * \brief Set the \f$\alpha\f$ shape parameter [1/Pa] of van Genuchten's
     *        curve.
     */
    void setVgAlpha(Scalar v)
    { vgAlpha_ = v; }

    /*!
     * \brief Return the \f$m\f$ shape parameter [-] of van Genuchten's
     *        curve.
     */
    Scalar vgm() const
    { return vgm_; }

    DUNE_DEPRECATED_MSG("use vgm() (uncapitalized 'm') instead")
    Scalar vgM() const
    { return vgm(); }

    /*!
     * \brief Set the \f$m\f$ shape parameter [-] of van Genuchten's
     *        curve.
     *
     * The \f$n\f$ shape parameter is set to \f$n = \frac{1}{1 - m}\f$
     */
    void setVgm(Scalar m)
    { vgm_ = m; vgn_ = 1/(1 - vgm_); }

    DUNE_DEPRECATED_MSG("use setVgm() (uncapitalized 'm') instead")
    void setVgM(Scalar m)
    { setVgm(m); }

    /*!
     * \brief Return the \f$n\f$ shape parameter [-] of van Genuchten's
     *        curve.
     */
    Scalar vgn() const
    { return vgn_; }

    DUNE_DEPRECATED_MSG("use vgn() (uncapitalized 'n') instead")
    Scalar vgN() const
    { return vgn(); }

    /*!
     * \brief Set the \f$n\f$ shape parameter [-] of van Genuchten's
     *        curve.
     *
     * The \f$n\f$ shape parameter is set to \f$m = 1 - \frac{1}{n}\f$
     */
    void setVgn(Scalar n)
    { vgn_ = n; vgm_ = 1 - 1/vgn_; }

    DUNE_DEPRECATED_MSG("use setVgn() (uncapitalized 'n') instead")
    void setVgN(Scalar n)
    { setVgn(n); }

private:
    Scalar vgAlpha_;
    Scalar vgm_;
    Scalar vgn_;
};
} // namespace Dumux

#endif
