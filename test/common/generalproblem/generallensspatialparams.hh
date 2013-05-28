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
 * \brief The spatial parameters for the GeneralLensProblem which uses the
 *        twophase box model or twophase decoupled model
 */
#ifndef DUMUX_GENERALLENSSPATIALPARAMS_HH
#define DUMUX_GENERALLENSSPATIALPARAMS_HH

#include <dumux/material/spatialparams/implicitspatialparams.hh>
#include <dumux/material/spatialparams/fvspatialparams.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedvangenuchten.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

#include <dumux/implicit/2p/2pmodel.hh>

namespace Dumux
{
//forward declaration
template<class TypeTag>
class GeneralLensSpatialParams;

namespace Properties
{
// The spatial parameters TypeTag
NEW_TYPE_TAG(GeneralLensSpatialParams);

// Property to define the spatial parameters base class -> allows switch with model switch!
NEW_PROP_TAG(SpatialParamsBaseClass);

// Set the spatial parameters
SET_TYPE_PROP(GeneralLensSpatialParams, SpatialParams, Dumux::GeneralLensSpatialParams<TypeTag>);

// Set the material Law
SET_PROP(GeneralLensSpatialParams, MaterialLaw)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef RegularizedVanGenuchten<Scalar> EffectiveLaw;
public:
    typedef EffToAbsLaw<EffectiveLaw> type;
};
}

/*!
 * \ingroup TwoPBoxModel
 * \ingroup IMPETtests
 *
 * \brief The spatial parameters for the LensProblem which uses the
 *        twophase box model or twophase decoupled model
 */
template<class TypeTag>
class GeneralLensSpatialParams : public GET_PROP_TYPE(TypeTag, SpatialParamsBaseClass)
{
    typedef typename GET_PROP_TYPE(TypeTag, SpatialParamsBaseClass) ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename Grid::ctype CoordScalar;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum {
        dim=GridView::dimension,
        dimWorld=GridView::dimensionworld,

        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx
    };

    typedef Dune::FieldVector<CoordScalar,dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar,dim,dim> FieldMatrix;

    typedef typename GridView::template Codim<0>::Entity Element;


public:
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    GeneralLensSpatialParams(const GridView& gridView)
        : ParentType(gridView)
    {
        try
        {
            lensLowerLeft_[0]   = GET_RUNTIME_PARAM(TypeTag, Scalar, SpatialParams.LensLowerLeftX);
            lensLowerLeft_[1]   = GET_RUNTIME_PARAM(TypeTag, Scalar, SpatialParams.LensLowerLeftY);
            lensUpperRight_[0]  = GET_RUNTIME_PARAM(TypeTag, Scalar, SpatialParams.LensUpperRightX);
            lensUpperRight_[1]  = GET_RUNTIME_PARAM(TypeTag, Scalar, SpatialParams.LensUpperRightY);
        }
        catch (Dumux::ParameterException &e) {
            std::cerr << e << ". Abort!\n";
            exit(1) ;
        }
        catch (...) {
            std::cerr << "Unknown exception thrown!\n";
            exit(1);
        }

        // residual saturations
        lensMaterialParams_.setSwr(0.18);
        lensMaterialParams_.setSnr(0.0);
        outerMaterialParams_.setSwr(0.05);
        outerMaterialParams_.setSnr(0.0);

        // parameters for the Van Genuchten law
        // alpha and n
        lensMaterialParams_.setVgAlpha(0.00045);
        lensMaterialParams_.setVgn(7.3);
        outerMaterialParams_.setVgAlpha(0.0037);
        outerMaterialParams_.setVgn(4.7);

        for (int i=0; i < dim; i++)
        {
        lensK_[i][i] = 9.05e-12;
        outerK_[i][i] = 4.6e-10;
        }
    }

        /*!
     * \brief Get the intrinsic permeability tensor
     *
     * \param globalPos The global coordinates of the finite volume
     */
    const FieldMatrix& intrinsicPermeabilityAtPos(const GlobalPosition &globalPos) const
    {
        if (isInLens_(globalPos))
            return lensK_;
        return outerK_;
    }

    /*!
     * \brief Get the porosity
     *
     * \param globalPos The global coordinates of the finite volume
     */
    Scalar porosityAtPos(const GlobalPosition &globalPos) const
    { return 0.4; }


    /*!
     * \brief Get the material law parameters
     *
     * \param globalPos The global coordinates of the finite volume
     *
     * \return the parameter object for the material law which depends on the position
     */
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition &globalPos) const
    {
        if (isInLens_(globalPos))
            return lensMaterialParams_;
        return outerMaterialParams_;
    }


private:
    bool isInLens_(const GlobalPosition &pos) const
    {
        for (int i = 0; i < dim; ++i) {
            if (pos[i] < lensLowerLeft_[i] || pos[i] > lensUpperRight_[i])
                return false;
        }
        return true;
    }

    GlobalPosition lensLowerLeft_;
    GlobalPosition lensUpperRight_;

    FieldMatrix lensK_;
    FieldMatrix outerK_;

    MaterialLawParams lensMaterialParams_;
    MaterialLawParams outerMaterialParams_;
};

} // end namespace
#endif

