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
 * \brief Base class for all problems
 */
#ifndef DUMUX_FV_PROBLEM_HH
#define DUMUX_FV_PROBLEM_HH

#include <memory>

#include <dune/common/version.hh>
#include <dune/common/fvector.hh>
#include <dune/grid/common/gridenums.hh>

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/discretization/methods.hh>

//#include <dumux/io/restart.hh>
//#include <dumux/implicit/adaptive/gridadapt.hh>

namespace Dumux
{
/*!
 * \ingroup Problems
 * \brief Base class for all finite-volume problems
 *
 * \note All quantities (regarding the units) are specified assuming a
 *       three-dimensional world. Problems discretized using 2D grids
 *       are assumed to be extruded by \f$1 m\f$ and 1D grids are assumed
 *       to have a cross section of \f$1m \times 1m\f$.
 */
template<class TypeTag>
class FVProblem
{
    using Implementation = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using ResidualVector = typename GET_PROP_TYPE(TypeTag, NumEqVector);
    using VertexMapper = typename GET_PROP_TYPE(TypeTag, VertexMapper);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using PointSource = typename GET_PROP_TYPE(TypeTag, PointSource);
    using PointSourceHelper = typename GET_PROP_TYPE(TypeTag, PointSourceHelper);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);

    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    using Element = typename GridView::template Codim<0>::Entity;
    using Vertex = typename GridView::template Codim<dim>::Entity;
    using CoordScalar = typename GridView::ctype;
    using GlobalPosition = Dune::FieldVector<CoordScalar, dimWorld>;

    static constexpr bool isBox = GET_PROP_VALUE(TypeTag, DiscretizationMethod) == DiscretizationMethods::Box;

    // using GridAdaptModel = ImplicitGridAdapt<TypeTag, adaptiveGrid>;

public:
    /*!
     * \brief Constructor
     *
     * \param gridView The simulation's idea about physical space
     */
    FVProblem(std::shared_ptr<const FVGridGeometry> fvGridGeometry)
    : fvGridGeometry_(fvGridGeometry)
    {
        // set a default name for the problem
        problemName_ = getParamFromGroup<std::string>(GET_PROP_VALUE(TypeTag, ModelParameterGroup), "Problem.Name");

        // TODO this has to be moved to the main file most probably
        // // if we are calculating on an adaptive grid get the grid adapt model
        // if (adaptiveGrid)
        //     gridAdapt_ = std::make_shared<GridAdaptModel>(asImp_());
        //     gridAdapt().init();

        // compute which scvs contain point sources
        computePointSourceMap(pointSourceMap_);
    }

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     * It could be either overwritten by the problem files, or simply
     * declared over the setName() function in the application file.
     */
    const std::string& name() const
    {
        return problemName_;
    }

    /*!
     * \brief Set the problem name.
     *
     * This static method sets the simulation name, which should be
     * called before the application problem is declared! If not, the
     * default name "sim" will be used.
     *
     * \param newName The problem's name
     */
    void setName(const std::string& newName)
    {
        problemName_ = newName;
    }

    /*!
     * \name Boundary conditions and sources defining the problem
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param element The finite element
     * \param scv The sub control volume
     */
    BoundaryTypes boundaryTypes(const Element &element,
                                const SubControlVolume &scv) const
    {
        if (!isBox)
            DUNE_THROW(Dune::InvalidStateException,
                       "boundaryTypes(..., scv) called for cell-centered method.");

        // forward it to the method which only takes the global coordinate
        return asImp_().boundaryTypesAtPos(scv.dofPosition());
    }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param element The finite element
     * \param scvf The sub control volume face
     */
    BoundaryTypes boundaryTypes(const Element &element,
                                const SubControlVolumeFace &scvf) const
    {
        if (isBox)
            DUNE_THROW(Dune::InvalidStateException,
                       "boundaryTypes(..., scvf) called for box method.");

        // forward it to the method which only takes the global coordinate
        return asImp_().boundaryTypesAtPos(scvf.ipGlobal());
    }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param globalPos The position of the finite volume in global coordinates
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        //! As a default, i.e. if the user's problem does not overload any boundaryTypes method
        //! set Dirichlet boundary conditions everywhere for all primary variables
        BoundaryTypes bcTypes;
        bcTypes.setAllDirichlet();
        return bcTypes;
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param element The finite element
     * \param scvf the sub control volume face
     *
     * The method returns the boundary types information.
     */
    PrimaryVariables dirichlet(const Element &element, const SubControlVolumeFace &scvf) const
    {
        // forward it to the method which only takes the global coordinate
        if (isBox)
        {
            DUNE_THROW(Dune::InvalidStateException, "dirichlet(scvf) called for box method.");
        }
        else
            return asImp_().dirichletAtPos(scvf.ipGlobal());
    }

    PrimaryVariables dirichlet(const Element &element, const SubControlVolume &scv) const
    {
        // forward it to the method which only takes the global coordinate
        if (!isBox)
        {
            DUNE_THROW(Dune::InvalidStateException, "dirichlet(scv) called for cell-centered method.");
        }
        else
            return asImp_().dirichletAtPos(scv.dofPosition());
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param globalPos The position of the center of the finite volume
     *            for which the dirichlet condition ought to be
     *            set in global coordinates
     *
     * For this method, the \a values parameter stores primary variables.
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no reasonable default value
        // for Dirichlet conditions)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem specifies that some boundary "
                   "segments are dirichlet, but does not provide "
                   "a dirichlet() or a dirichletAtPos() method.");
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * This is the method for the case where the Neumann condition is
     * potentially solution dependent and requires some quantities that
     * are specific to the fully-implicit method.
     *
     * \param values The neumann values for the conservation equations in units of
     *                 \f$ [ \textnormal{unit of conserved quantity} / (m^2 \cdot s )] \f$
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry
     * \param elemVolVars All volume variables for the element
     * \param scvf The sub control volume face
     *
     * For this method, the \a values parameter stores the flux
     * in normal direction of each phase. Negative values mean influx.
     * E.g. for the mass balance that would the mass flux in \f$ [ kg / (m^2 \cdot s)] \f$.
     */
    ResidualVector neumann(const Element& element,
                           const FVElementGeometry& fvGeometry,
                           const ElementVolumeVariables& elemVolvars,
                           const SubControlVolumeFace& scvf) const
    {
        // forward it to the interface with only the global position
        return asImp_().neumannAtPos(scvf.ipGlobal());
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param globalPos The position of the boundary face's integration point in global coordinates
     *
     * For this method, the \a values parameter stores the flux
     * in normal direction of each phase. Negative values mean influx.
     * E.g. for the mass balance that would be the mass flux in \f$ [ kg / (m^2 \cdot s)] \f$.
     */
    ResidualVector neumannAtPos(const GlobalPosition &globalPos) const
    {
        //! As a default, i.e. if the user's problem does not overload any neumann method
        //! return no-flow Neumann boundary conditions at all Neumann boundaries
        return ResidualVector(0.0);
    }

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * This is the method for the case where the source term is
     * potentially solution dependent and requires some quantities that
     * are specific to the fully-implicit method.
     *
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry
     * \param elemVolVars All volume variables for the element
     * \param scv The sub control volume
     *
     * For this method, the \a values parameter stores the conserved quantity rate
     * generated or annihilate per volume unit. Positive values mean
     * that the conserved quantity is created, negative ones mean that it vanishes.
     * E.g. for the mass balance that would be a mass rate in \f$ [ kg / (m^3 \cdot s)] \f$.
     */
    ResidualVector source(const Element &element,
                          const FVElementGeometry& fvGeometry,
                          const ElementVolumeVariables& elemVolVars,
                          const SubControlVolume &scv) const
    {
        // forward to solution independent, fully-implicit specific interface
        return asImp_().sourceAtPos(scv.center());
    }

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * \param globalPos The position of the center of the finite volume
     *            for which the source term ought to be
     *            specified in global coordinates
     *
     * For this method, the \a values parameter stores the conserved quantity rate
     * generated or annihilate per volume unit. Positive values mean
     * that the conserved quantity is created, negative ones mean that it vanishes.
     * E.g. for the mass balance that would be a mass rate in \f$ [ kg / (m^3 \cdot s)] \f$.
     */
    ResidualVector sourceAtPos(const GlobalPosition &globalPos) const
    {
        //! As a default, i.e. if the user's problem does not overload any source method
        //! return 0.0 (no source terms)
        return ResidualVector(0.0);
    }

    /*!
     * \brief Applies a vector of point sources. The point sources
     *        are possibly solution dependent.
     *
     * \param pointSources A vector of PointSource s that contain
              source values for all phases and space positions.
     *
     * For this method, the \a values method of the point source
     * has to return the absolute rate values in units
     * \f$ [ \textnormal{unit of conserved quantity} / s ] \f$.
     * Positive values mean that the conserved quantity is created, negative ones mean that it vanishes.
     * E.g. for the mass balance that would be a mass rate in \f$ [ kg / s ] \f$.
     */
    void addPointSources(std::vector<PointSource>& pointSources) const {}

    /*!
     * \brief Evaluate the point sources (added by addPointSources)
     *        for all phases within a given sub-control-volume.
     *
     * This is the method for the case where the point source is
     * solution dependent and requires some quantities that
     * are specific to the fully-implicit method.
     *
     * \param source A single point source
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry
     * \param elemVolVars All volume variables for the element
     * \param scv The sub control volume
     *
     * For this method, the \a values() method of the point sources returns
     * the absolute conserved quantity rate generated or annihilate in
     * units \f$ [ \textnormal{unit of conserved quantity} / s ] \f$.
     * Positive values mean that the conserved quantity is created, negative ones mean that it vanishes.
     * E.g. for the mass balance that would be a mass rate in \f$ [ kg / s ] \f$.
     */
    void pointSource(PointSource& source,
                     const Element &element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const SubControlVolume &scv) const
    {
        // forward to space dependent interface method
        asImp_().pointSourceAtPos(source, source.position());
    }

    /*!
     * \brief Evaluate the point sources (added by addPointSources)
     *        for all phases within a given sub-control-volume.
     *
     * This is the method for the case where the point source is space dependent
     *
     * \param pointSource A single point source
     * \param globalPos The point source position in global coordinates
     *
     * For this method, the \a values() method of the point sources returns
     * the absolute conserved quantity rate generated or annihilate in
     * units \f$ [ \textnormal{unit of conserved quantity} / s ] \f$. Positive values mean
     * that the conserved quantity is created, negative ones mean that it vanishes.
     * E.g. for the mass balance that would be a mass rate in \f$ [ kg / s ] \f$.
     */
    void pointSourceAtPos(PointSource& pointSource,
                          const GlobalPosition &globalPos) const {}

    /*!
     * \brief Adds contribution of point sources for a specific sub control volume
     *        to the values.
     *        Caution: Only overload this method in the implementation if you know
     *                 what you are doing.
     */
    ResidualVector scvPointSources(const Element &element,
                                   const FVElementGeometry& fvGeometry,
                                   const ElementVolumeVariables& elemVolVars,
                                   const SubControlVolume &scv) const
    {
        ResidualVector source(0);
        auto scvIdx = scv.indexInElement();
        auto key = std::make_pair(fvGridGeometry_->gridView().indexSet().index(element), scvIdx);
        if (pointSourceMap_.count(key))
        {
            // call the solDependent function. Herein the user might fill/add values to the point sources
            // we make a copy of the local point sources here
            auto pointSources = pointSourceMap_.at(key);

            // Add the contributions to the dof source values
            // We divide by the volume. In the local residual this will be multiplied with the same
            // factor again. That's because the user specifies absolute values in kg/s.
            const Scalar volume = scv.volume()*elemVolVars[scv].extrusionFactor();

            for (auto&& pointSource : pointSources)
            {
                // Note: two concepts are implemented here. The PointSource property can be set to a
                // customized point source function achieving variable point sources,
                // see TimeDependentPointSource for an example. The second imitated the standard
                // dumux source interface with solDependentPointSource / pointSourceAtPos, methods
                // that can be overloaded in the actual problem class also achieving variable point sources.
                // The first one is more convenient for simple function like a time dependent source.
                // The second one might be more convenient for e.g. a solution dependent point source.

                // we do an update e.g. used for TimeDependentPointSource
                pointSource.update(asImp_(), element, fvGeometry, elemVolVars, scv);
                // call convienience problem interface function
                asImp_().pointSource(pointSource, element, fvGeometry, elemVolVars, scv);
                // at last take care about multiplying with the correct volume
                pointSource /= volume;
                // add the point source values to the local residual
                source += pointSource.values();
            }
        }

        return source;
    }

    //! Compute the point source map, i.e. which scvs have point source contributions
    template<class PointSourceMap>
    void computePointSourceMap(PointSourceMap& pointSourceMap)
    {
        // clear the given point source maps in case it's not empty
        pointSourceMap.clear();

        // get and apply point sources if any given in the problem
        std::vector<PointSource> sources;
        asImp_().addPointSources(sources);

        // if there are point sources compute the DOF to point source map
        if (!sources.empty())
        {
            // calculate point source locations and save them in a map
            PointSourceHelper::computePointSourceMap(*fvGridGeometry_,
                                                     sources,
                                                     pointSourceMap);
        }
    }

    /*!
     * \brief Applies the initial solution for all degrees of freedom of the grid.
     *
    */
    void applyInitialSolution(SolutionVector& sol) const
    {
        // set the initial values by forwarding to a specialized method
        applyInitialSolutionImpl_(sol, std::integral_constant<bool, isBox>());
    }

    /*!
     * \brief Evaluate the initial value for
     * an element (for cell-centered models)
     * or vertex (for box / vertex-centered models)
     *
     * \param entity The dof entity (element or vertex)
     */
    template<class Entity>
    PrimaryVariables initial(const Entity& entity) const
    {
        static_assert(int(Entity::codimension) == 0 || int(Entity::codimension) == dim, "Entity must be element or vertex");
        return asImp_().initialAtPos(entity.geometry().center());
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param globalPos The global position
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no reasonable default value
        // for initial values)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "an initial() or an initialAtPos() method.");
    }

    /*!
     * \brief Return how much the domain is extruded at a given sub-control volume.
     *
     * This means the factor by which a lower-dimensional (1D or 2D)
     * entity needs to be expanded to get a full dimensional cell. The
     * default is 1.0 which means that 1D problems are actually
     * thought as pipes with a cross section of 1 m^2 and 2D problems
     * are assumed to extend 1 m to the back.
     */
    Scalar extrusionFactor(const Element &element,
                           const SubControlVolume &scv,
                           const ElementSolutionVector &elemSol) const
    {
        // forward to generic interface
        return asImp_().extrusionFactorAtPos(scv.center());
    }

    /*!
     * \brief Return how much the domain is extruded at a given position.
     *
     * This means the factor by which a lower-dimensional (1D or 2D)
     * entity needs to be expanded to get a full dimensional cell. The
     * default is 1.0 which means that 1D problems are actually
     * thought as pipes with a cross section of 1 m^2 and 2D problems
     * are assumed to extend 1 m to the back.
     */
    Scalar extrusionFactorAtPos(const GlobalPosition &globalPos) const
    {
        //! As a default, i.e. if the user's problem does not overload any extrusion factor method
        //! return 1.0
        return 1.0;
    }

    // \}

    /*!
     * \name Simulation steering
     */
    // \{

    /*!
     * \brief Called by the time manager before the time integration.
     */
    // TODO most likely move to the main file
    // void preTimeStep()
    // {
    //     // If adaptivity is used, this method adapts the grid.
    //     // Remeber to call the parent class function if this is overwritten
    //     // on a lower problem level when using an adaptive grid
    //     if (adaptiveGrid && timeManager().timeStepIndex() > 0)
    //     {
    //         this->gridAdapt().adaptGrid();

    //         // if the grid changed recompute the source map and the bounding box tree
    //         if (asImp_().gridChanged())
    //         {
    //             computePointSourceMap();
    //         }
    //     }
    // }

    /*!
     * \brief TODO serialization
     */
    // void timeIntegration()
    // {
    //     // if the simulation  run is about to abort, write restart files for the current and previous time steps:
    //     // write restart file for the current time step
    //     serialize();

    //     //write restart file for the previous time step:
    //     //set the time manager and the solution vector to the previous time step
    //     const Scalar time = timeManager().time();
    //     timeManager().setTime(time - timeManager().previousTimeStepSize());
    //     const auto curSol = model_.curSol();
    //     model_.curSol() = model_.prevSol();
    //     //write restart file
    //     serialize();
    //     //reset time manager and solution vector
    //     model_.curSol() = curSol;
    //     timeManager().setTime(time);
    // }

    // TODO could be move to the episode manager that is user implemented?
    // /*!
    //  * \brief Called when the end of an simulation episode is reached.
    //  *
    //  * Typically a new episode should be started in this method.
    //  */
    // void episodeEnd()
    // {
    //     std::cerr << "The end of an episode is reached, but the problem "
    //               << "does not override the episodeEnd() method. "
    //               << "Doing nothing!\n";
    // }
    // \}

    /*!
     * \name TODO: Restart mechanism
     */
    // \{

    /*!
     * \brief This method writes the complete state of the simulation
     *        to the harddisk.
     *
     * The file will start with the prefix returned by the name()
     * method, has the current time of the simulation clock in it's
     * name and uses the extension <tt>.drs</tt>. (Dumux ReStart
     * file.)  See Restart for details.
     */
    // void serialize()
    // {
    //     typedef Restart Restarter;
    //     Restarter res;
    //     res.serializeBegin(asImp_());
    //     if (gridView().comm().rank() == 0)
    //         std::cout << "Serialize to file '" << res.fileName() << "'\n";

    //     timeManager().serialize(res);
    //     asImp_().serialize(res);
    //     res.serializeEnd();
    // }

    /*!
     * \brief This method writes the complete state of the problem
     *        to the harddisk.
     *
     * The file will start with the prefix returned by the name()
     * method, has the current time of the simulation clock in it's
     * name and uses the extension <tt>.drs</tt>. (Dumux ReStart
     * file.)  See Restart for details.
     *
     * \tparam Restarter The serializer type
     *
     * \param res The serializer object
     */
    // template <class Restarter>
    // void serialize(Restarter &res)
    // {
    //     vtkOutputModule_->serialize(res);
    //     model().serialize(res);
    // }

    /*!
     * \brief Load a previously saved state of the whole simulation
     *        from disk.
     *
     * \param tRestart The simulation time on which the program was
     *                 written to disk.
     */
    // void restart(const Scalar tRestart)
    // {
    //     typedef Restart Restarter;

    //     Restarter res;

    //     res.deserializeBegin(asImp_(), tRestart);
    //     if (gridView().comm().rank() == 0)
    //         std::cout << "Deserialize from file '" << res.fileName() << "'\n";
    //     timeManager().deserialize(res);
    //     asImp_().deserialize(res);
    //     res.deserializeEnd();
    // }

    /*!
     * \brief This method restores the complete state of the problem
     *        from disk.
     *
     * It is the inverse of the serialize() method.
     *
     * \tparam Restarter The deserializer type
     *
     * \param res The deserializer object
     */
    // template <class Restarter>
    // void deserialize(Restarter &res)
    // {
    //     vtkOutputModule_->deserialize(res);
    //     model().deserialize(res);
    // }

    // \}

    // TODO this can probably be a setter function for the assembler
    /*!
     * \brief Function to add additional DOF dependencies, i.e. the residual of DOF globalIdx depends
     * on additional DOFs not included in the discretization schemes' occupation pattern
     *
     * \param globalIdx The index of the DOF that depends on additional DOFs
     * \return A vector of the additional DOFs the DOF with index globalIdx depends on
     *
     * \note This will lead to additional matrix entries and derivative computations automatically
     *       This function is used when creating the matrix and when computing entries of the jacobian matrix
     * Per default we don't have additional DOFs
     */
    // std::vector<IndexType> getAdditionalDofDependencies(IndexType globalIdx) const
    // { return std::vector<IndexType>(); }

    // TODO is this necessary or can it just be meshed?
    /*!
     * \brief Function to set intersections as interior boundaries. This functionality is only
     *        available for models using cell-centered schemes. The corresponding boundary
     *        types and conditions are obtained from the standard methods.
     *
     * \param element The finite element
     * \param intersection The intersection within the element
     * \return boolean to mark an intersection as an interior boundary
     *
     * Per default we don't have interior boundaries
     */
    // template<class T = TypeTag>
    // bool isInteriorBoundary(const Element& element, const Intersection& intersection) const
    // { return false; }

    //! The finite volume grid geometry
    const FVGridGeometry& fvGridGeometry() const
    { return *fvGridGeometry_; }

protected:
    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    { return *static_cast<Implementation *>(this); }

    //! \copydoc asImp_()
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *>(this); }

private:
    /*!
     * \brief Applies the initial solution for the box method
     */
    void applyInitialSolutionImpl_(SolutionVector& sol, /*isBox=*/std::true_type) const
    {
        for (const auto& vertex : vertices(fvGridGeometry_->gridView()))
        {
            const auto dofIdxGlobal = fvGridGeometry_->vertexMapper().index(vertex);
            sol[dofIdxGlobal] = asImp_().initial(vertex);
        }
    }

    /*!
     * \brief Applies the initial solution for cell-centered methods
     */
    void applyInitialSolutionImpl_(SolutionVector& sol, /*isBox=*/std::false_type) const
    {
        for (const auto& element : elements(fvGridGeometry_->gridView()))
        {
            const auto dofIdxGlobal = fvGridGeometry_->elementMapper().index(element);
            sol[dofIdxGlobal] = asImp_().initial(element);
        }
    }

    //! The finite volume grid geometry
    std::shared_ptr<const FVGridGeometry> fvGridGeometry_;

    //! The name of the problem
    std::string problemName_;

    //! A map from an scv to a vector of point sources
    std::map<std::pair<unsigned int, unsigned int>,
             std::vector<PointSource> > pointSourceMap_;
};

} // end namespace Dumux

#endif