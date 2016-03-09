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
 * \brief This file contains the data which is required to calculate
 *        diffusive mass fluxes due to molecular diffusion with Fick's law.
 */
#ifndef DUMUX_CC_TPFA_FICKS_LAW_HH
#define DUMUX_CC_TPFA_FICKS_LAW_HH

#include <dune/common/float_cmp.hh>

#include <dumux/common/math.hh>
#include <dumux/common/parameters.hh>

#include <dumux/implicit/properties.hh>


namespace Dumux
{

namespace Properties
{
// forward declaration of properties
NEW_PROP_TAG(NumPhases);
NEW_PROP_TAG(FluidSystem);
NEW_PROP_TAG(EffectiveDiffusivityModel);
}

/*!
 * \ingroup CCTpfaFicksLaw
 * \brief Evaluates the diffusive mass flux according to Fick's law
 */
template <class TypeTag>
class CCTpfaFicksLaw
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, EffectiveDiffusivityModel) EffDiffModel;
    typedef typename GET_PROP_TYPE(TypeTag, SubControlVolume) SubControlVolume;
    typedef typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace) SubControlVolumeFace;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::IndexSet::IndexType IndexType;

    enum { dim = GridView::dimension} ;
    enum { dimWorld = GridView::dimensionworld} ;
    enum { numPhases = GET_PROP_VALUE(TypeTag, NumPhases)} ;

    typedef Dune::FieldMatrix<Scalar, dimWorld, dimWorld> DimWorldMatrix;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

public:

    void update(const Problem &problem, const SubControlVolumeFace &scvFace, int phaseIdx, int compIdx)
    {
        problemPtr_ = &problem;
        scvFacePtr_ = &scvFace;

        phaseIdx_ = phaseIdx;
        compIdx_ = compIdx;

        updateStencil_();

        // TODO for non solution dependent diffusion tensors...
    }

    /*!
     * \brief A function to calculate the mass flux over a sub control volume face
     *
     * \param phaseIdx The index of the phase of which the flux is to be calculated
     * \param compIdx The index of the transported component
     */
    Scalar flux()
    {
        // diffusion tensors are always solution dependent
        updateTransmissibilities_();

        const auto insideScvIdx = scvFace_().insideScvIdx();
        const auto& insideScv = problem_().model().fvGeometries().subControlVolume(insideScvIdx);

        // Set the inside-/outside volume variables provisionally
        const auto &insideVolVars = problem_().model().curVolVars(insideScv);
        VolumeVariables tmp;
        VolumeVariables& outsideVolVars = tmp;

        // calculate the concentration in the inside cell
        Scalar cInside = insideVolVars.molarDensity(phaseIdx_)*insideVolVars.moleFraction(phaseIdx_, compIdx_);


        Scalar cOutside;
        if (!scvFace_().boundary())
        {
            outsideVolVars = problem_().model().curVolVars(scvFace_().outsideScvIdx());
            cOutside = outsideVolVars.molarDensity(phaseIdx_)*outsideVolVars.moleFraction(phaseIdx_, compIdx_);
        }
        else
        {
            // if (complexBCTreatment)
            // outsideVolVars = problem_().model().constructBoundaryVolumeVariables(scvFace_());
            // else
            // {
                auto element = problem_().model().fvGeometries().element(insideScv);
                auto dirichletPriVars = problem_().dirichlet(element, scvFace_());
                outsideVolVars.update(dirichletPriVars, problem_(), element, insideScv);
            // }
            cOutside = outsideVolVars.molarDensity(phaseIdx_)*outsideVolVars.moleFraction(phaseIdx_, compIdx_);
        }

        return tij_*(cInside - cOutside);
    }

    std::set<IndexType> stencil() const
    {
        return std::set<IndexType>(stencil_.begin(), stencil_.end());
    }

protected:


    void updateTransmissibilities_()
    {
        const auto insideScvIdx = scvFace_().insideScvIdx();
        const auto& insideScv = problem_().model().fvGeometries().subControlVolume(insideScvIdx);
        const auto& insideVolVars = problem_().model().curVolVars(insideScvIdx);

        auto insideD = insideVolVars.diffusionCoefficient(phaseIdx_, compIdx_);
        insideD = EffDiffModel::effectiveDiffusivity(insideVolVars.porosity(), insideVolVars.saturation(phaseIdx_), insideD);
        Scalar ti = calculateOmega_(insideD, insideScv);

        if (!scvFace_().boundary())
        {
            const auto outsideScvIdx = scvFace_().outsideScvIdx();
            const auto& outsideScv = problem_().model().fvGeometries().subControlVolume(outsideScvIdx);
            const auto& outsideVolVars = problem_().model().curVolVars(outsideScvIdx);

            auto outsideD = outsideVolVars.diffusionCoefficient(phaseIdx_, compIdx_);
            outsideD = EffDiffModel::effectiveDiffusivity(outsideVolVars.porosity(), outsideVolVars.saturation(phaseIdx_), outsideD);
            Scalar tj = -1.0*calculateOmega_(outsideD, outsideScv);

            tij_ = scvFace_().area()*(ti * tj)/(ti + tj);
        }
        else
        {
            tij_ = scvFace_().area()*ti;
        }
    }

    void updateStencil_()
    {
        // fill the stencil
        if (!scvFace_().boundary())
            stencil_= {scvFace_().insideVolVarsIdx(), scvFace_().outsideVolVarsIdx()};
        else
            // fill the stencil
            stencil_ = {scvFace_().insideVolVarsIdx()};
    }

    Scalar calculateOmega_(const DimWorldMatrix &D, const SubControlVolume &scv) const
    {
        GlobalPosition Dnormal;
        D.mv(scvFace_().unitOuterNormal(), Dnormal);

        auto distanceVector = scvFace_().center();
        distanceVector -= scv.center();
        distanceVector /= distanceVector.two_norm2();

        Scalar omega = Dnormal * distanceVector;
        omega *= problem_().model().curVolVars(scv).extrusionFactor();

        return omega;
    }

    Scalar calculateOmega_(Scalar D, const SubControlVolume &scv) const
    {
        auto distanceVector = scvFace_().center();
        distanceVector -= scv.center();
        distanceVector /= distanceVector.two_norm2();

        Scalar omega = D * (distanceVector * scvFace_().unitOuterNormal());
        omega *= problem_().model().curVolVars(scv).extrusionFactor();

        return omega;
    }

    const Problem &problem_() const
    {
        return *problemPtr_;
    }

    const SubControlVolumeFace& scvFace_() const
    {
        return *scvFacePtr_;
    }

    const Problem *problemPtr_;
    const SubControlVolumeFace *scvFacePtr_; //!< Pointer to the sub control volume face for which the flux variables are created
    Scalar tij_;                             //!< transmissibility for the flux calculation
    std::vector<IndexType> stencil_;         //!< Indices of the cells of which the pressure is needed for the flux calculation
    IndexType phaseIdx_;
    IndexType compIdx_;
};

} // end namespace

#endif