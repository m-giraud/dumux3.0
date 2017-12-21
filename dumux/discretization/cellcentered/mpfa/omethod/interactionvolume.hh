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
 * \ingroup CCMpfaDiscretization
 * \brief Class for the interaction volume of the mpfa-o scheme.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_O_INTERACTIONVOLUME_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_O_INTERACTIONVOLUME_HH

#include <dune/common/dynmatrix.hh>
#include <dune/common/dynvector.hh>
#include <dune/common/fvector.hh>

#include <dumux/common/math.hh>
#include <dumux/common/properties.hh>

#include <dumux/discretization/cellcentered/mpfa/interactionvolumedatahandle.hh>
#include <dumux/discretization/cellcentered/mpfa/interactionvolumebase.hh>
#include <dumux/discretization/cellcentered/mpfa/dualgridindexset.hh>
#include <dumux/discretization/cellcentered/mpfa/localfacedata.hh>
#include <dumux/discretization/cellcentered/mpfa/methods.hh>

#include "localsubcontrolentities.hh"
#include "interactionvolumeindexset.hh"

namespace Dumux
{
//! Forward declaration of the interaction volume specialization for the mpfa-o scheme
template< class TypeTag >
class CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod>;

//! Specialization of the interaction volume traits class for the mpfa-o method
template< class TypeTag >
struct CCMpfaInteractionVolumeTraits< CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod> >
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);

    static constexpr int dim = GridView::dimension;
    static constexpr int dimWorld = GridView::dimensionworld;

    using InteractionVolumeType = CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod>;

public:
    //! Per default, we use the same interaction volume everywhere (also on boundaries etc...)
    using SecondaryInteractionVolume = InteractionVolumeType;

    //! export the type used for local indices
    using LocalIndexType = std::uint8_t;
    //! export the type used for indices on the grid
    using GridIndexType = typename GridView::IndexSet::IndexType;
    //! export the type for the interaction volume index set
    using IndexSet = CCMpfaOInteractionVolumeIndexSet< DualGridNodalIndexSet<GridIndexType, LocalIndexType, dim> >;
    //! export the type used for global coordinates
    using GlobalPosition = Dune::FieldVector< typename GridView::ctype, dimWorld >;
    //! export the type of interaction-volume local scvs
    using LocalScvType = CCMpfaOInteractionVolumeLocalScv< IndexSet, GlobalPosition, dim >;
    //! export the type of interaction-volume local scvfs
    using LocalScvfType = CCMpfaOInteractionVolumeLocalScvf< IndexSet >;
    //! export the type of used for the iv-local face data
    using LocalFaceData = InteractionVolumeLocalFaceData<GridIndexType, LocalIndexType>;
    //! export the type of face data container (use dynamic container here)
    using LocalFaceDataContainer = std::vector< LocalFaceData >;
    //! export the type used for iv-local matrices
    using Matrix = Dune::DynamicMatrix< Scalar >;
    //! export the type used for iv-local vectors
    using Vector = Dune::DynamicVector< Scalar >;
    //! export the type used for the iv-stencils
    using Stencil = std::vector< GridIndexType >;
    //! export the data handle type for this iv
    using DataHandle = InteractionVolumeDataHandle< TypeTag, InteractionVolumeType >;
};

/*!
 * \ingroup CCMpfaDiscretization
 * \brief Class for the interaction volume of the mpfa-o method.
 */
template<class TypeTag>
class CCMpfaInteractionVolumeImplementation< TypeTag, MpfaMethods::oMethod >
           : public CCMpfaInteractionVolumeBase< TypeTag,
                                                 CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod> >
{
    using ThisType = CCMpfaInteractionVolumeImplementation< TypeTag, MpfaMethods::oMethod >;
    using ParentType = CCMpfaInteractionVolumeBase< TypeTag, ThisType >;

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Helper = typename GET_PROP_TYPE(TypeTag, MpfaHelper);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;

    static constexpr int dim = GridView::dimension;
    using DimVector = Dune::FieldVector<Scalar, dim>;

    using TraitsType = CCMpfaInteractionVolumeTraits< ThisType >;
    using Matrix = typename TraitsType::Matrix;
    using LocalScvType = typename TraitsType::LocalScvType;
    using LocalScvfType = typename TraitsType::LocalScvfType;
    using LocalFaceData = typename TraitsType::LocalFaceData;

    using IndexSet = typename TraitsType::IndexSet;
    using GridIndexType = typename TraitsType::GridIndexType;
    using LocalIndexType = typename TraitsType::LocalIndexType;
    using Stencil = typename TraitsType::Stencil;

    //! Data attached to scvf touching Dirichlet boundaries.
    //! For the default o-scheme, we only store the corresponding vol vars index.
    class DirichletData
    {
        GridIndexType volVarIndex_;

    public:
        //! Constructor
        DirichletData(const GridIndexType index) : volVarIndex_(index) {}

        //! Return corresponding vol var index
        GridIndexType volVarIndex() const { return volVarIndex_; }
    };

public:
    //! publicly state the mpfa-scheme this interaction volume is associated with
    static constexpr MpfaMethods MpfaMethod = MpfaMethods::oMethod;

    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);

    //! Sets up the local scope for a given iv index set
    void setUpLocalScope(const IndexSet& indexSet,
                         const Problem& problem,
                         const FVElementGeometry& fvGeometry)
    {
        // for the o-scheme, the stencil is equal to the scv
        // index set of the dual grid's nodal index set
        stencil_ = &indexSet.nodalIndexSet().globalScvIndices();

        // number of interaction-volume-local (= node-local for o-scheme) scvs/scvf
        numFaces_ = indexSet.numFaces();
        const auto numLocalScvs = indexSet.numScvs();
        const auto numGlobalScvfs = indexSet.nodalIndexSet().numScvfs();

        // reserve memory for local entities
        elements_.clear();
        scvs_.clear();
        scvfs_.clear();
        localFaceData_.clear();
        dirichletData_.clear();
        elements_.reserve(numLocalScvs);
        scvs_.reserve(numLocalScvs);
        scvfs_.reserve(numFaces_);
        dirichletData_.reserve(numFaces_);
        localFaceData_.reserve(numGlobalScvfs);

        // set up quantities related to sub-control volumes
        const auto& scvIndices = indexSet.globalScvIndices();
        for (LocalIndexType scvIdxLocal = 0; scvIdxLocal < numLocalScvs; scvIdxLocal++)
        {
            scvs_.emplace_back(Helper(),
                               fvGeometry,
                               fvGeometry.scv( scvIndices[scvIdxLocal] ),
                               scvIdxLocal,
                               indexSet);
            elements_.emplace_back(fvGeometry.fvGridGeometry().element( scvIndices[scvIdxLocal] ));
        }

        // keep track of the number of unknowns etc
        numUnknowns_ = 0;
        numKnowns_ = numLocalScvs;

        // resize omega storage container
        wijk_.resize(numFaces_);

        // set up quantitites related to sub-control volume faces
        for (LocalIndexType faceIdxLocal = 0; faceIdxLocal < numFaces_; ++faceIdxLocal)
        {
            // get corresponding grid scvf
            const auto& scvf = fvGeometry.scvf(indexSet.scvfIdxGlobal(faceIdxLocal));

            // the neighboring scvs in local indices (order: 0 - inside scv, 1..n - outside scvs)
            const auto& neighborScvIndicesLocal = indexSet.neighboringLocalScvIndices(faceIdxLocal);

            // create local face data object for this face
            localFaceData_.emplace_back(faceIdxLocal, neighborScvIndicesLocal[0], scvf.index());

            // create iv-local scvf object
            if (scvf.boundary())
            {
                const auto bcTypes = problem.boundaryTypes(elements_[neighborScvIndicesLocal[0]], scvf);

                if (bcTypes.hasOnlyDirichlet())
                {
                    scvfs_.emplace_back(scvf, neighborScvIndicesLocal, numKnowns_++, /*isDirichlet*/true);
                    dirichletData_.emplace_back(scvf.outsideScvIdx());
                }
                else
                    scvfs_.emplace_back(scvf, neighborScvIndicesLocal, numUnknowns_++, /*isDirichlet*/false);

                // on boundary faces we will only need one inside omega
                wijk_[faceIdxLocal].resize(1);
            }
            else
            {
                scvfs_.emplace_back(scvf, neighborScvIndicesLocal, numUnknowns_++, /*isDirichlet*/false);

                // we will need as many omegas as scvs around the face
                const auto numNeighborScvs = neighborScvIndicesLocal.size();
                wijk_[faceIdxLocal].resize(numNeighborScvs);

                // add local face data objects for the outside faces
                for (LocalIndexType i = 1; i < numNeighborScvs; ++i)
                {
                    // loop over scvfs in outside scv until we find the one coinciding with current scvf
                    const auto outsideLocalScvIdx = neighborScvIndicesLocal[i];
                    for (int coord = 0; coord < dim; ++coord)
                    {
                        if (indexSet.scvfIdxLocal(outsideLocalScvIdx, coord) == faceIdxLocal)
                        {
                            const auto globalScvfIdx = indexSet.nodalIndexSet().scvfIdxGlobal(outsideLocalScvIdx, coord);
                            const auto& flipScvf = fvGeometry.scvf(globalScvfIdx);
                            localFaceData_.emplace_back(faceIdxLocal,       // iv-local scvf idx
                                                        outsideLocalScvIdx, // iv-local scv index
                                                        i-1,                // scvf-local index in outside faces
                                                        flipScvf.index());  // global scvf index
                        }
                    }
                }
            }
        }

        // Resize local matrices
        A_.resize(numUnknowns_, numUnknowns_);
        B_.resize(numUnknowns_, numKnowns_);
        C_.resize(numFaces_, numUnknowns_);
    }

    //! returns the number of primary scvfs of this interaction volume
    std::size_t numFaces() const { return numFaces_; }

    //! returns the number of intermediate unknowns within this interaction volume
    std::size_t numUnknowns() const { return numUnknowns_; }

    //! returns the number of (in this context) known solution values within this interaction volume
    std::size_t numKnowns() const { return numKnowns_; }

    //! returns the number of scvs embedded in this interaction volume
    std::size_t numScvs() const { return scvs_.size(); }

    //! returns the number of scvfs embedded in this interaction volume
    std::size_t numScvfs() const { return scvfs_.size(); }

    //! returns the cell-stencil of this interaction volume
    const Stencil& stencil() const { return *stencil_; }

    //! returns the grid element corresponding to a given iv-local scv idx
    const Element& element(const LocalIndexType ivLocalScvIdx) const { return elements_[ivLocalScvIdx]; }

    //! returns the local scvf entity corresponding to a given iv-local scvf idx
    const LocalScvfType& localScvf(const LocalIndexType ivLocalScvfIdx) const { return scvfs_[ivLocalScvfIdx]; }

    //! returns the local scv entity corresponding to a given iv-local scv idx
    const LocalScvType& localScv(const LocalIndexType ivLocalScvfIdx) const { return scvs_[ivLocalScvfIdx]; }

    //! returns a reference to the container with the local face data
    const std::vector<LocalFaceData>& localFaceData() const { return localFaceData_; }

    //! returns a reference to the information container on Dirichlet BCs within this iv
    const std::vector<DirichletData>& dirichletData() const { return dirichletData_; }

    //! returns the matrix associated with face unknowns in local equation system
    const Matrix& A() const { return A_; }
    Matrix& A() { return A_; }

    //! returns the matrix associated with cell unknowns in local equation system
    const Matrix& B() const { return B_; }
    Matrix& B() { return B_; }

    //! returns the matrix associated with face unknowns in flux expressions
    const Matrix& C() const { return C_; }
    Matrix& C() { return C_; }

    //! returns container storing the transmissibilities for each face & coordinate
    const std::vector< std::vector< DimVector > >& omegas() const { return wijk_; }
    std::vector< std::vector< DimVector > >& omegas() { return wijk_; }

    //! returns the number of interaction volumes living around a vertex
    //! the mpfa-o scheme always constructs one iv per vertex
    template< class NodalIndexSet >
    static std::size_t numInteractionVolumesAtVertex(const NodalIndexSet& nodalIndexSet) { return 1; }

    //! adds the iv index sets living around a vertex to a given container
    //! and stores the the corresponding index in a map for each scvf
    template<class IvIndexSetContainer, class ScvfIndexMap, class NodalIndexSet>
    static void addInteractionVolumeIndexSets(IvIndexSetContainer& ivIndexSetContainer,
                                              ScvfIndexMap& scvfIndexMap,
                                              const NodalIndexSet& nodalIndexSet)
    {
        // the global index of the iv index set that is about to be created
        const auto curGlobalIndex = ivIndexSetContainer.size();

        // make the one index set for this node
        ivIndexSetContainer.emplace_back(nodalIndexSet);

        // store the index mapping
        for (const auto scvfIdx : nodalIndexSet.globalScvfIndices())
            scvfIndexMap[scvfIdx] = curGlobalIndex;
    }

private:
    // pointer to cell stencil (in iv index set)
    const Stencil* stencil_;

    // Variables defining the local scope
    std::vector<Element> elements_;
    std::vector<LocalScvType> scvs_;
    std::vector<LocalScvfType> scvfs_;
    std::vector<LocalFaceData> localFaceData_;
    std::vector<DirichletData> dirichletData_;

    // Matrices needed for computation of transmissibilities
    Matrix A_;
    Matrix B_;
    Matrix C_;

    // The omega factors are stored during assemble of local system
    std::vector< std::vector< DimVector > > wijk_;

    // sizes involved in the local system equations
    std::size_t numFaces_;
    std::size_t numUnknowns_;
    std::size_t numKnowns_;
};

} // end namespace

#endif
