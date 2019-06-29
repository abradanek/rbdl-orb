/*
 * RBDL - Rigid Body Dynamics Library
 * Copyright (c) 2011-2018 Martin Felis <martin@fysx.org>
 *
 * Licensed under the zlib license. See LICENSE for more details.
 */

#ifndef RBDL_CONSTRAINTS_H
#define RBDL_CONSTRAINTS_H

#include <rbdl/rbdl_math.h>
#include <rbdl/rbdl_mathutils.h>

namespace RigidBodyDynamics {

/** \page constraints_page Constraints
 *
 * All functions related to contacts are specified in the \ref
 * constraints_group "Constraints Module".

 * \defgroup constraints_group Constraints
 *
 * Constraints are handled by specification of a \link
 * RigidBodyDynamics::ConstraintSet
 * ConstraintSet \endlink which contains all informations about the
 * current constraints and workspace memory.
 *
 * Separate constraints can be specified by calling
 * ConstraintSet::AddContactConstraint(), ConstraintSet::AddLoopConstraint(),
 * and ConstraintSet::AddCustomConstraint().
 * After all constraints have been specified, this \link
 * RigidBodyDynamics::ConstraintSet
 * ConstraintSet \endlink has to be bound to the model via
 * ConstraintSet::Bind(). This initializes workspace memory that is
 * later used when calling one of the contact functions, such as
 * ForwardDynamicsContactsDirects().
 *
 * The values in the vectors ConstraintSet::force and
 * ConstraintSet::impulse contain the computed force or
 * impulse values for each constraint when returning from one of the
 * contact functions.
 *
 * \section solution_constraint_system Solution of the Constraint System
 *
 * \subsection constraint_system Linear System of the Constrained Dynamics
 *
 * In the presence of constraints, to compute the
 * acceleration one has to solve a linear system of the form: \f[
 \left(
   \begin{array}{cc}
     H & G^T \\
     G & 0
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     \ddot{q} \\
     - \lambda
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     -C + \tau \\
     \gamma
   \end{array}
 \right)
 * \f] where \f$H\f$ is the joint space inertia matrix computed with the
 * CompositeRigidBodyAlgorithm(), \f$G\f$ is the constraint jacobian,
 * \f$C\f$ the bias force (sometimes called "non-linear
 * effects"), and \f$\gamma\f$ the generalized acceleration independent
 * part of the constraints.
 *
 * \subsection collision_system Linear System of the Contact Collision
 *
 * Similarly to compute the response of the model to a contact gain one has
 * to solve a system of the following form: \f[
 \left(
   \begin{array}{cc}
     H & G^T \\
     G & 0
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     \dot{q}^{+} \\
     \Lambda
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     H \dot{q}^{-} \\
    v^{+} 
   \end{array}
 \right)
 * \f] where \f$H\f$ is the joint space inertia matrix computed with the
 * CompositeRigidBodyAlgorithm(), \f$G\f$ are the point jacobians of the
 * contact points, \f$\dot{q}^{+}\f$ the generalized velocity after the
 * impact, \f$\Lambda\f$ the impulses at each constraint, \f$\dot{q}^{-}\f$
 * the generalized velocity before the impact, and \f$v^{+}\f$ the desired
 * velocity of each constraint after the impact (known beforehand, usually
 * 0). The value of \f$v^{+}\f$ can is specified via the variable
 * ConstraintSet::v_plus and defaults to 0.
 *
 * \subsection solution_methods Solution Methods
 *
 * There are essentially three different approaches to solve these systems:
 * -# \b Direct: solve the full system to simultaneously compute
 *  \f$\ddot{q}\f$ and \f$\lambda\f$. This may be slow for large systems
 *  and many constraints.
 * -# \b Range-Space: solve first for \f$\lambda\f$ and then for
 *  \f$\ddot{q}\f$.
 * -# \b Null-Space: solve furst for \f$\ddot{q}\f$ and then for
 *  \f$\lambda\f$
 * The methods are the same for the contact gains just with different
 * variables on the right-hand-side.
 *
 * RBDL provides methods for all approaches. The implementation for the
 * range-space method also exploits sparsities in the joint space inertia
 * matrix using a sparse structure preserving \f$L^TL\f$ decomposition as
 * described in Chapter 8.5 of "Rigid Body Dynamics Algorithms".
 *
 * None of the methods is generally superior to the others and each has
 * different trade-offs as factors such as model topology, number of
 * constraints, constrained bodies, numerical stability, and performance
 * vary and evaluation has to be made on a case-by-case basis.
 * 
 * \subsection solving_constraints_dynamics Methods for Solving Constrained 
 * Dynamics
 * 
 * RBDL provides the following methods to compute the acceleration of a
 * constrained system:
 *
 * - ForwardDynamicsConstraintsDirect()
 * - ForwardDynamicsConstraintsRangeSpaceSparse()
 * - ForwardDynamicsConstraintsNullSpace()
 *
 * \subsection solving_constraints_collisions Methods for Computing Collisions
 *
 * RBDL provides the following methods to compute the collision response on
 * new contact gains:
 *
 * - ComputeConstraintImpulsesDirect()
 * - ComputeConstraintImpulsesRangeSpaceSparse()
 * - ComputeConstraintImpulsesNullSpace()
 *
 * \subsection assembly_q_qdot Computing generalized joint positions and velocities
 * satisfying the constraint equations.
 *
 * When considering a model subject position level constraints expressed by the
 * equation \f$\phi (q) = 0\f$, it is often necessary to compute generalized joint
 * position and velocities which satisfy the constraints. Even velocity-level
 * constraints may have position-level assembly constraints:  a rolling-without-slipping
 * constraint is a velocity-level constraint, but during assembly it might be desireable
 * to put the rolling surfaces in contact with eachother.
 *
 * In order to compute a vector of generalized joint positions that satisfy
 * the constraints it is necessary to solve the following optimization problem:
 * \f{eqnarray*}{
 * \text{minimize} && \sum_{i = 0}^{n} (q - q_{0})^T W (q - q_{0}) \\
 * \text{over} && q \\
 * \text{subject to} && \phi (q) = 0
 * \f}
 * 
 * In order to compute a vector of generalized joint velocities that satisfy
 * the constraints it is necessary to solve the following optimization problem:
 * \f{eqnarray*}{
 * \text{minimize} && \sum_{i = 0}^{n} (\dot{q} - \dot{q}_{0})^T W (\dot{q} - \dot{q}_{0}) \\
 * \text{over} && \dot{q} \\
 * \text{subject to} && \dot{\phi} (q) = \phi _{q}(q) \dot{q} + \phi _{t}(q) = 0
 * \f}
 *
 * \f$q_{0}\f$ and \f$\dot{q}_{0}\f$ are initial guesses, \f$\phi _{q}\f$ is the
 * constraints jacobian (partial derivative of \f$\phi\f$ with respect to \f$q\f$),
 * and \f$\phi _{t}(q)\f$ is the partial derivative of \f$\phi\f$ with respect
 * to time.  \f$W\f$ is a diagonal weighting matrix, which can be exploited
 * to prioritize changes in the position/ velocity of specific joints.
 * With a solver capable of handling singular matrices, it is possible to set to
 * 1 the weight of the joint positions/ velocities that should not be changed
 * from the initial guesses, and to 0 those corresponding to the values that
 * can be changed.
 *
 * These problems are solved using the Lagrange multipliers method. For the
 * velocity problem the solution is exact. For the position problem the
 * constraints are linearized in the form 
 * \f$ \phi (q_{0}) + \phi _{t}(q0) + \phi _{q_0}(q) (q - q_{0}) \f$
 * and the linearized problem is solved iteratively until the constraint position
 * errors are smaller than a given threshold.
 *
 * RBDL provides two functions to compute feasible joint position and velocities:
 * - CalcAssemblyQ()
 * - CalcAssemblyQDot()
 *
 * \subsection baumgarte_stabilization Baumgarte Stabilization
 *
 * The constrained dynamic equations are correct at the acceleration level
 * but will drift at the velocity and position level during numerical 
 * integration. RBDL implements Baumgarte stabilization to avoid
 * the accumulation of position and velocity errors for loop constraints and
 * custom constraints. Contact constraints do not have Baumgarte stabilization
 * because they are a special case which does not typically suffer from drift. 
 * The stabilization term can be enabled/disabled using the appropriate 
 * ConstraintSet::AddLoopConstraint and ConstraintSet::AddCustomConstraint 
 * functions. 
 *
 * The dynamic equations are changed to the following form: \f[
 \left(
   \begin{array}{cc}
     H & G^T \\
     G & 0
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     \ddot{q} \\
     - \lambda
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     -C + \tau \\
     \gamma + \gamma _{stab}
   \end{array}
 \right)
 * \f] A term \f$\gamma _{stab}\f$ is added to the right hand side of the
 * equation, defined in the following way: \f[
   \gamma _{stab} = - 2 \alpha \dot{\phi} (q) - \beta^2 \phi (q)
 * \f] where \f$\phi (q)\f$ are the position level constraint errors and 
 * \f$\alpha\f$ and \f$\beta\f$ are tuning coefficients. There is
 * no clear and simple rule on how to choose them as good values also
 * depend on the used integration method and time step. If the values are
 * too small the constrained dynamics equation becomes stiff, too big
 * values result in errors not being reduced.
 *
 * A good starting point is to parameterize the coefficients as:
 * \f[
 * \alpha = \beta = 1 / T_\textit{stab}
 * \f]
 * with \f$T_\textit{stab}\f$ specifies a time constant for errors in position
 * and velocity errors to reduce. Featherstone suggests in his book "Rigid
 * Body Dynamics Algorithms" that for a big industrial robot a value of 0.1
 * is reasonable. When testing different values best is to try different
 * orders of magnitude as e.g. doubling a value only has little effect.
 *
 * For Loop- and CustomConstraints Baumgarte stabilization is enabled by
 * default and uses the stabilization parameter \f$T_\textit{stab} = 0.1\f$.
 *
 * @{
 */

struct Model;

struct RBDL_DLLAPI CustomConstraint;

/** \brief Structure that contains both constraint information and workspace memory.
 *
 * This structure is used to reduce the amount of memory allocations that
 * are needed when computing constraint forces.
 *
 * The ConstraintSet has to be bound to a model using ConstraintSet::Bind()
 * before it can be used in \link RigidBodyDynamics::ForwardDynamicsContacts
 * ForwardDynamicsContacts \endlink.
 */
struct RBDL_DLLAPI ConstraintSet {
  ConstraintSet() :
    linear_solver (Math::LinearSolverColPivHouseholderQR),
    bound (false) {}

  // Enum to describe the type of a constraint.
  enum ConstraintType {
    ContactConstraint,
    LoopConstraint,
    ConstraintTypeCustom,
    ConstraintTypeLast,
  };

  /** \brief Adds a contact constraint to the constraint set.
   *
   * This type of constraints ensures that the velocity and acceleration of a specified
   * body point along a specified axis are null. This constraint does not act
   * at the position level.
   *
   * \param body_id the body which is affected directly by the constraint
   * \param body_point the point that is constrained relative to the
   * contact body
   * \param world_normal the normal along the constraint acts (in base
   * coordinates)
   * \param constraint_name a human readable name (optional, default: NULL)
   * \param normal_acceleration the acceleration of the contact along the normal
   * (optional, default: 0.)
   */
  unsigned int AddContactConstraint (
    unsigned int body_id,
    const Math::Vector3d &body_point,
    const Math::Vector3d &world_normal,
    const char *constraint_name = NULL,
    double normal_acceleration = 0.);

  /** \brief Adds a loop constraint to the constraint set.
   *
   * This type of constraints ensures that the relative orientation and position,
   * spatial velocity, and spatial acceleration between two frames in two bodies
   * are null along a specified spatial constraint axis.
   *
   * \param id_predecessor the identifier of the predecessor body
   * \param id_successor the identifier of the successor body
   * \param X_predecessor a spatial transform localizing the constrained
   * frames on the predecessor body, expressed with respect to the predecessor
   * body frame
   * \param X_successor a spatial transform localizing the constrained
   * frames on the successor body, expressed with respect to the successor
   * body frame
   * \param axis a spatial vector indicating the axis along which the constraint
   * acts
   * \param enable_stabilization Whether \ref baumgarte_stabilization
   * should be enabled or not.
   * \param stabilization_param The value for \f$T_\textit{stab}\f$ used for the
   * \ref baumgarte_stabilization (defaults to 0.1).
   * \param constraint_name a human readable name (optional, default: NULL)
   *
   */
  unsigned int AddLoopConstraint(
    unsigned int id_predecessor, 
    unsigned int id_successor,
    const Math::SpatialTransform &X_predecessor,
    const Math::SpatialTransform &X_successor,
    const Math::SpatialVector &axis,
    bool enable_stabilization = false,
    const double stabilization_param = 0.1,
    const char *constraint_name = NULL
    );

  /** \brief Adds a custom constraint to the constraint set.
   *
   * \param custom_constraint The CustomConstraint to be added to the
   * ConstraintSet.
   * \param id_predecessor the identifier of the predecessor body
   * \param id_successor the identifier of the successor body
   * \param X_predecessor a spatial transform localizing the constrained
   * frames on the predecessor body, expressed with respect to the predecessor
   * body frame
   * \param X_successor a spatial transform localizing the constrained
   * frames on the successor body, expressed with respect to the successor
   * body frame
   * \param axis a spatial vector indicating the axis along which the constraint
   * acts
   * \param enable_stabilization Whether \ref baumgarte_stabilization
   * should be enabled or not.
   * \param stabilization_param The value for \f$T_\textit{stab}\f$ used for the
   * \ref baumgarte_stabilization (defaults to 0.1).
   * \param constraint_name a human readable name (optional, default: NULL)
   *
   */
  unsigned int AddCustomConstraint(
    CustomConstraint* custom_constraint,
    unsigned int id_predecessor,
    unsigned int id_successor,
    const Math::SpatialTransform &X_predecessor,
    const Math::SpatialTransform &X_successor,
    bool enable_stabilization = false,
    const double stabilization_param = 0.1,
    const char *constraint_name = NULL
    );


  /** \brief Copies the constraints and resets its ConstraintSet::bound
   * flag.
   */
  ConstraintSet Copy() {
    ConstraintSet result (*this);
    result.bound = false;

    return result;
  }

  /** \brief Specifies which method should be used for solving undelying linear systems.
   */
  void SetSolver (Math::LinearSolver solver) {
    linear_solver = solver;
  }

  /** \brief Initializes and allocates memory for the constraint set.
   *
   * This function allocates memory for temporary values and matrices that
   * are required for contact force computation. Both model and constraint
   * set must not be changed after a binding as the required memory is
   * dependent on the model size (i.e. the number of bodies and degrees of
   * freedom) and the number of constraints in the Constraint set.
   *
   * The values of ConstraintSet::acceleration may still be
   * modified after the set is bound to the model.
   *
   */
  bool Bind (const Model &model);

  /**
    \brief Initializes and allocates memory needed for 
            InverseDynamicsConstraints and InverseDynamicsConstraintsRelaxed

    This function allocates the temporary vectors and matrices needed
    for the RigidBodyDynamics::InverseDynamicsConstraints and RigidBodyDynamics::InverseDynamicsConstraintsRelaxed
    methods. In addition, the constant matrices S and P are set here. 
    This function needs to be called once
    before calling either RigidBodyDynamics::InverseDynamicsConstraints or
    RigidBodyDynamics::InverseDynamicsConstraintsRelaxed. It does not ever need be called
    again, unless the actuated degrees of freedom change, or the constraint
    set changes.

    \param model rigid body model   
    \param actuatedDof : a vector that is q_size in length (or dof_count in 
                         length) which has a 'true' entry for every generalized 
                         degree-of freedom that is driven by an actuator and 
                         'false' for every degree-of-freedom that is not. 

  */
  void SetActuationMap(const Model &model,
                          const std::vector<bool> &actuatedDof);

  /** \brief Returns the number of constraints. */
  size_t size() const {
    return acceleration.size();
  }

  /** \brief Clears all variables in the constraint set. */
  void clear ();

  /// Method that should be used to solve internal linear systems.
  Math::LinearSolver linear_solver;
  /// Whether the constraint set was bound to a model (mandatory!).
  bool bound;

  // Common constraints variables.
  std::vector<ConstraintType> constraintType;
  std::vector<std::string> name;
  std::vector<unsigned int> mContactConstraintIndices;
  std::vector<unsigned int> mLoopConstraintIndices;
  std::vector<unsigned int> mCustomConstraintIndices;
  std::vector< CustomConstraint* > mCustomConstraints;

  // Contact constraints variables.
  std::vector<unsigned int> body;
  std::vector<Math::Vector3d> point;
  std::vector<Math::Vector3d> normal;

  // Loop constraints variables.
  std::vector<unsigned int> body_p;
  std::vector<unsigned int> body_s;
  std::vector<Math::SpatialTransform> X_p;
  std::vector<Math::SpatialTransform> X_s;
  std::vector<Math::SpatialVector> constraintAxis;
  /** Baumgarte stabilization parameter */
  std::vector<Math::Vector2d> baumgarteParameters;
  /** Position error for the Baumgarte stabilization */
  Math::VectorNd err;
  /** Velocity error for the Baumgarte stabilization */
  Math::VectorNd errd;

  /** Enforced accelerations of the contact points along the contact
   * normal. */
  Math::VectorNd acceleration;
  /** Actual constraint forces along the contact normals. */
  Math::VectorNd force;
  /** Actual constraint impulses along the contact normals. */
  Math::VectorNd impulse;
  /** The velocities we want to have along the contact normals */
  Math::VectorNd v_plus;

  // Variables used by the Lagrangian methods

  /// Workspace for the joint space inertia matrix.
  Math::MatrixNd H;
  /// Workspace for the coriolis forces.
  Math::VectorNd C;
  /// Workspace of the lower part of b.
  Math::VectorNd gamma;
  Math::MatrixNd G;
  /// Workspace for the Lagrangian left-hand-side matrix.
  Math::MatrixNd A;
  /// Workspace for the Lagrangian right-hand-side.
  Math::VectorNd b;
  /// Workspace for the Lagrangian solution.
  Math::VectorNd x;

  /// Selection matrix for the actuated parts of the model needed
  /// for the inverse-dynamics-with-constraints operator
  Math::MatrixNd S;
  /// Selection matrix for the non-actuated parts of the model
  Math::MatrixNd P;
  /// Matrix that holds the relative cost of deviating from the desired
  /// accelerations
  Math::MatrixNd W;
  Math::MatrixNd Winv;
  Math::VectorNd u;
  Math::VectorNd v;

  Math::MatrixNd F;
  Math::MatrixNd GT;
  Math::VectorNd g;
  Math::MatrixNd Ru;
  Math::VectorNd py;
  Math::VectorNd pz;

  /// Workspace when evaluating contact Jacobians
  Math::MatrixNd Gi;
  /// Workspace when evaluating loop/CustomConstraint Jacobians
  Math::MatrixNd GSpi;
  /// Workspace when evaluating loop/CustomConstraint Jacobians
  Math::MatrixNd GSsi;
  /// Workspace when evaluating loop/CustomConstraint Jacobians
  Math::MatrixNd GSJ;

  /// Workspace for the QR decomposition of the null-space method
#ifdef RBDL_USE_SIMPLE_MATH
  SimpleMath::HouseholderQR<Math::MatrixNd> GT_qr;
#else
  Eigen::HouseholderQR<Math::MatrixNd> GT_qr;
  Eigen::FullPivHouseholderQR<Math::MatrixNd> GPT_full_qr;
#endif

  Math::MatrixNd GT_qr_Q;
  Math::MatrixNd GPT;
  Math::MatrixNd Y;
  Math::MatrixNd Z;
  Math::MatrixNd R;  
  Math::VectorNd qddot_y;
  Math::VectorNd qddot_z;

  Math::MatrixNd AIdc;
  Math::MatrixNd KIdc;
  Math::VectorNd bIdc;
  Math::VectorNd xIdc;
  Math::VectorNd vIdc;
  Math::VectorNd wIdc;
  // Variables used by the IABI methods

  /// Workspace for the Inverse Articulated-Body Inertia.
  Math::MatrixNd K;
  /// Workspace for the accelerations of due to the test forces
  Math::VectorNd a;
  /// Workspace for the test accelerations.
  Math::VectorNd QDDot_t;
  /// Workspace for the default accelerations.
  Math::VectorNd QDDot_0;
  /// Workspace for the test forces.
  std::vector<Math::SpatialVector> f_t;
  /// Workspace for the actual spatial forces.
  std::vector<Math::SpatialVector> f_ext_constraints;
  /// Workspace for the default point accelerations.
  std::vector<Math::Vector3d> point_accel_0;

  /// Workspace for the bias force due to the test force
  std::vector<Math::SpatialVector> d_pA;
  /// Workspace for the acceleration due to the test force
  std::vector<Math::SpatialVector> d_a;
  Math::VectorNd d_u;

  /// Workspace for the inertia when applying constraint forces
  std::vector<Math::SpatialMatrix> d_IA;
  /// Workspace when applying constraint forces
  std::vector<Math::SpatialVector> d_U;
  /// Workspace when applying constraint forces
  Math::VectorNd d_d;

  std::vector<Math::Vector3d> d_multdof3_u;

  //CustomConstraint variables.

  
};

/** \brief Computes the position errors for the given ConstraintSet.
  *
  * \param model the model
  * \param Q the generalized positions of the joints
  * \param CS the constraint set for which the error should be computed
  * \param err (output) vector where the error will be stored in (should have
  * the size of CS).
  * \param update_kinematics whether the kinematics of the model should be
  * updated from Q.
  *
  * \note the position error is always 0 for contact constraints.
  *
  */
RBDL_DLLAPI
void CalcConstraintsPositionError(
  Model& model,
  const Math::VectorNd &Q,
  ConstraintSet &CS,
  Math::VectorNd& err,
  bool update_kinematics = true
);

/** \brief Computes the Jacobian for the given ConstraintSet
 *
 * \param model the model
 * \param Q     the generalized positions of the joints
 * \param CS    the constraint set for which the Jacobian should be computed
 * \param G     (output) matrix where the output will be stored in
 * \param update_kinematics whether the kinematics of the model should be 
 * updated from Q
 *
 */
RBDL_DLLAPI
void CalcConstraintsJacobian(
  Model &model,
  const Math::VectorNd &Q,
  ConstraintSet &CS,
  Math::MatrixNd &G,
  bool update_kinematics = true
);

/** \brief Computes the velocity errors for the given ConstraintSet.
  *
  *
  * \param model the model
  * \param Q the generalized positions of the joints
  * \param QDot the generalized velocities of the joints
  * \param CS the constraint set for which the error should be computed
  * \param err (output) vector where the error will be stored in (should have
  * the size of CS).
  * \param update_kinematics whether the kinematics of the model should be
  * updated from Q.
  *
  * \note this is equivalent to multiplying the constraint jacobian by the 
  * generalized velocities of the joints.
  *
  */
RBDL_DLLAPI
void CalcConstraintsVelocityError(
  Model& model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  ConstraintSet &CS,
  Math::VectorNd& err,
  bool update_kinematics = true
);

/** \brief Computes the terms \f$H\f$, \f$G\f$, and \f$\gamma\f$ of the 
  * constrained dynamic problem and stores them in the ConstraintSet.
  *
  *
  * \param model the model
  * \param Q the generalized positions of the joints
  * \param QDot the generalized velocities of the joints
  * \param Tau the generalized forces of the joints
  * \param CS the constraint set for which the error should be computed
  * \param f_ext External forces acting on the body in base coordinates (optional, defaults to NULL)
  *
  * \note This function is normally called automatically in the various
  * constrained dynamics functions, the user normally does not have to call it.
  *
  */
RBDL_DLLAPI
void CalcConstrainedSystemVariables (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  const Math::VectorNd &Tau,
  ConstraintSet &CS,
  std::vector<Math::SpatialVector> *f_ext = NULL
);

/** \brief Computes a feasible initial value of the generalized joint positions.
  * 
  * \param model the model
  * \param QInit initial guess for the generalized positions of the joints
  * \param CS the constraint set for which the error should be computed
  * \param Q (output) vector of the generalized joint positions.
  * \param weights weighting coefficients for the different joint positions.
  * \param tolerance the function will return successfully if the constraint
  * position error norm is lower than this value.
  * \param max_iter the funciton will return unsuccessfully after performing
  * this number of iterations.
  * 
  * \return true if the generalized joint positions were computed successfully,
  * false otherwise.f
  *
  */
RBDL_DLLAPI
bool CalcAssemblyQ(
  Model &model,
  Math::VectorNd QInit,
  ConstraintSet &CS,
  Math::VectorNd &Q,
  const Math::VectorNd &weights,
  double tolerance = 1e-12,
  unsigned int max_iter = 100
);

/** \brief Computes a feasible initial value of the generalized joint velocities.
  * 
  * \param model the model
  * \param Q the generalized joint position of the joints. It is assumed that
  * this vector satisfies the position level assemblt constraints.
  * \param QDotInit initial guess for the generalized velocities of the joints
  * \param CS the constraint set for which the error should be computed
  * \param QDot (output) vector of the generalized joint velocities.
  * \param weights weighting coefficients for the different joint positions.
  *
  */
RBDL_DLLAPI
void CalcAssemblyQDot(
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDotInit,
  ConstraintSet &CS,
  Math::VectorNd &QDot,
  const Math::VectorNd &weights
);

/** \brief Computes forward dynamics with contact by constructing and solving 
 *  the full lagrangian equation
 *
 * This method builds and solves the linear system \f[
 \left(
   \begin{array}{cc}
     H & G^T \\
     G & 0
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     \ddot{q} \\
     -\lambda
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     -C + \tau \\
     \gamma
   \end{array}
 \right)
 * \f] where \f$H\f$ is the joint space inertia matrix computed with the
 * CompositeRigidBodyAlgorithm(), \f$G\f$ are the point jacobians of the
 * contact points, \f$C\f$ the bias force (sometimes called "non-linear
 * effects"), and \f$\gamma\f$ the generalized acceleration independent
 * part of the contact point accelerations.
 *
 * \note This function works with ContactConstraints, LoopConstraints and
 * Custom Constraints. Nonetheless, this method will not tolerate redundant
 * constraints.
 * 
 * \par 
 *
 * \note To increase performance group constraints body and pointwise such
 * that constraints acting on the same body point are sequentially in
 * ConstraintSet. This can save computation of point jacobians \f$G\f$.
 *
 * \param model rigid body model
 * \param Q     state vector of the internal joints
 * \param QDot  velocity vector of the internal joints
 * \param Tau   actuations of the internal joints
 * \param CS    the description of all acting constraints
 * \param QDDot accelerations of the internals joints (output)
 * \param f_ext External forces acting on the body in base coordinates (optional, defaults to NULL)
 * \note During execution of this function values such as
 * ConstraintSet::force get modified and will contain the value
 * of the force acting along the normal.
 *
 */
RBDL_DLLAPI
void ForwardDynamicsConstraintsDirect (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  const Math::VectorNd &Tau,
  ConstraintSet &CS,
  Math::VectorNd &QDDot,
  std::vector<Math::SpatialVector> *f_ext = NULL
);

RBDL_DLLAPI
void ForwardDynamicsConstraintsRangeSpaceSparse (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  const Math::VectorNd &Tau,
  ConstraintSet &CS,
  Math::VectorNd &QDDot,
  std::vector<Math::SpatialVector> *f_ext = NULL
);

RBDL_DLLAPI
void ForwardDynamicsConstraintsNullSpace (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  const Math::VectorNd &Tau,
  ConstraintSet &CS,
  Math::VectorNd &QDDot,
  std::vector<Math::SpatialVector> *f_ext = NULL
);

/** \brief Computes forward dynamics that accounts for active contacts in 
 *  ConstraintSet.
 *
 * The method used here is the one described by Kokkevis and Metaxas in the
 * Paper "Practical Physics for Articulated Characters", Game Developers
 * Conference, 2004.
 *
 * It does this by recursively computing the inverse articulated-body inertia (IABI)
 * \f$\Phi_{i,j}\f$ which is then used to build and solve a system of the form:
 \f[
 \left(
   \begin{array}{c}
     \dot{v}_1 \\
     \dot{v}_2 \\
     \vdots \\
     \dot{v}_n
   \end{array}
 \right)
 =
 \left(
   \begin{array}{cccc}
     \Phi_{1,1} & \Phi_{1,2} & \cdots & \Phi{1,n} \\
     \Phi_{2,1} & \Phi_{2,2} & \cdots & \Phi{2,n} \\
     \cdots & \cdots & \cdots & \vdots \\
     \Phi_{n,1} & \Phi_{n,2} & \cdots & \Phi{n,n} 
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     f_1 \\
     f_2 \\
     \vdots \\
     f_n
   \end{array}
 \right)
 + 
 \left(
   \begin{array}{c}
   \phi_1 \\
   \phi_2 \\
   \vdots \\
   \phi_n
   \end{array}
 \right).
 \f]
 Here \f$n\f$ is the number of constraints and the method for building the system
 uses the Articulated Body Algorithm to efficiently compute entries of the system. The
 values \f$\dot{v}_i\f$ are the constraint accelerations, \f$f_i\f$ the constraint forces,
 and \f$\phi_i\f$ are the constraint bias forces.
 *
 * \param model rigid body model
 * \param Q     state vector of the internal joints
 * \param QDot  velocity vector of the internal joints
 * \param Tau   actuations of the internal joints
 * \param CS a list of all contact points
 * \param QDDot accelerations of the internals joints (output)
 *
 * \note During execution of this function values such as
 * ConstraintSet::force get modified and will contain the value
 * of the force acting along the normal.
 *
 * \note This function supports only contact constraints.
 *
 * \todo Allow for external forces
 */
RBDL_DLLAPI
void ForwardDynamicsContactsKokkevis (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDot,
  const Math::VectorNd &Tau,
  ConstraintSet &CS,
  Math::VectorNd &QDDot
);



/**
 @brief A relaxed inverse-dynamics operator that can be applied to
        under-actuated or fully-actuated constrained multibody systems.
  \par
  <b>Important</b>
  Set the actuated degrees-of-freedom using RigidBodyDynamics::ConstraintSet::SetActuationMap
  <b>prior</b> to calling this function.
  \par
  This function implements the relaxed inverse-dynamics operator defined by
  Koch [1] and Kudruss [2]. When given a vector of
  generalized positions, generalized velocities, and desired generalized
  accelerations will solve for a set of generalized accelerations and forces
  which satisfy the constrained equations of motion such that the solution 
  is close to a vector of desired acceleration controls \f$x\f$ 
  \f[
   \min{\ddot{q}} \dfrac{1}{2} \ddot{q}^T H \ddot{q} + C^T \ddot{q} + \dfrac{1}{2}(Sx-S\ddot{q})^{T} W (Sx-S\ddot{q})
  \f]
  s.t.
  \f[
   G \ddot{q} = \gamma.
  \f]
  In contrast to the
  RigidBodyDynamics::InverseDynamicsConstraints method, this method can work
  with underactuated systems. Mathematically this method does not depend on
  \f[
    \text{rank}(GP^T) < n-n_a
  \f]
  where \f$n\f$ is the number of degrees of freedom and \f$n_a\f$ is the
  number of actuated degrees of freedom. 
  \par
  For those readers who are unfamiliar with quadratic programs (QP), like the 
  constrained minimization problem above, read the following important notes.
  One consequence of this additional flexibility is that the term \f$x\f$ should 
  now be interpreted as a control vector: 

  -# The minimum of the above constrained QP may not be \f$x = S\ddot{q}^*\f$ 
  where \f$\ddot{q}^*\f$ is the vector of desired accelerations. The terms 
  \f$\dfrac{1}{2} \ddot{q}^T H \ddot{q}\f$ will \f$C^T \ddot{q}\f$ pull
  the solution away from this value and the constraint \f$G \ddot{q} = \gamma\f$
  may make it impossible to exactly satisfy \f$S\ddot{q}*=S\ddot{q}\f$.   
  -# Koch's original formulation has been modifed so that setting 
  \f$x = \ddot{q}^*\f$ will yield a solution for \f$\ddot{q}\f$ that is close 
  to \f$\ddot{q}^*\f$. However, even if an exact solution for 
  \f$\ddot{q} = \ddot{q}^*\f$ exists it will may not be realized using
  \f$x = \ddot{q}*\f$. Iteration may be required.

  To solve the above constrained minimization problem we take the derivative
  of the above system of equations w.r.t. \f$\ddot{q}\f$ and \f$\lambda\f$
  set the result to zero and solve. This results in the KKT system
 \f[ 
    \left( \begin{array}{cc}
      H+K & G^T \\
      G & 0
    \end{array} \right)
    \left( \begin{array}{c}
    \ddot{q} \\
    -\lambda
    \end{array} \right)
    =\left(
    \begin{array}{c}
       (S^T W S)x - C\\
       \gamma
    \end{array}
    \right).
  \f]
  This system of linear equations is not solved directly, but instead
  the null-space formulation presented in Sec. 2.5 of
  Koch as it is much faster. 
  As with the RigidBodyDynamics::InverseDynamicsConstraints method the
  matrices \f$S\f$ and \f$P\f$ select the actuated and unactuated parts of
  \f[\ddot{q} = S^T u + P^T v
  \f], 
  and
  \f[u^* = S^T x
  \f],
  where \f$ S \f$ is a selection matrix that returns the actuated subspace of
  \f$ \ddot{q} \f$ (\f$ S\ddot{q} \f$) and \f$ P \f$ returns the unactuated
  subspace of \f$ \ddot{q} \f$ (\f$ P\ddot{q} \f$).
  \f[
   \left(
     \begin{array}{ccc}
      S H S^T+W & S M P^T & S G^T \\
      P M S^T & P M P^T & P G^T \\
      G S^T & G P^T & 0
     \end{array}
   \right)
   \left(
     \begin{array}{c}
      u \\
      v \\
      -\lambda
     \end{array}
   \right)
   =
   \left(
     \begin{array}{c}
       Wu^* -SC\\
       -PC\\
      \gamma
     \end{array}
   \right)
  \f]
  This system has an upper block triangular structure which can be seen by
  noting that
  \f[ 
    J^T = \left( \begin{array}{c} S G^T \\ P G^T \end{array} \right),
  \f]
  by grouping the upper \f$2 \times 2\f$ block into
  \f[
  F = \left( \begin{array}{cc} 
              SMS^T + W & SMP^T \\ 
              PMS^T & PMP^T \end{array} \right),
  \f]
  and by grouping the right hand side into
  \f[
    g = \left( \begin{array}{c} -Wu^* + SC \\ PC \end{array} \right)
  \f]
  resulting in
  \f[
    \left( \begin{array}{cc}
            F & J^T \\
            J & 0
            \end{array}
    \right)
    \left( \begin{array}{c}
              p \\
             -\lambda 
            \end{array}
    \right)
    =
  \left( \begin{array}{c}
              -g \\
              \gamma 
            \end{array}
    \right)        
  \f]
  This system can be triangularized by projecting the system into the null
  space of \f$G^T\f$. First we begin with a QR decomposition of \f$G^T\f$ into
  \f[ 
    J^T = \left( Y \, Z \right)\left( \begin{array}{c} R \\ 0 \end{array} \right)
  \f]
  and projecting \f$(u,v)\f$ into the space \f$[Y,Z]\f$ 
  \f[
    p = Y p_Y + Z p_Z.
  \f]
  This allows us to express the previous KKT system as
  \f[
    \left( \begin{array}{ccc}
            Y^T F Y & Y^T F Z & R \\
            Z^T F Y & Z^T F Z & 0 \\
            R^T & 0 & 0  
            \end{array}
    \right)
    \left( \begin{array}{c}
              p_Y \\
              p_Z \\
             -\lambda 
            \end{array}
    \right)
    =
  \left( \begin{array}{c}
              -Y^T g \\
             -Z^T g \\ 
             \gamma 
            \end{array}
    \right)        
  \f]
  Though this system is still \f$(n+c) \times (n+c)\f$ it can be solved in parts
  for \f$p_Y\f$
  \f[
    R^T p_Y = \gamma,
  \f]
  and \f$p_Z\f$
  \f[
    (Z^T F Z) p_Z = -(Z^T F Y)p_Y - Z^T g
  \f]
  which is enough to yield a solution for 
  \f[\left(
     \begin{array}{c}
      u \\ 
      v
     \end{array} \right) = (Y p_Y + Z p_Z)
  \f]
  and finally 
  \f[
    \ddot{q} = S^T u + P^T v.
  \f]
  This method is less computationally expensive than the KKT system directly since  
  \f$R\f$ is of size \f$ c \times c \f$ and \f$ (Z^T F Z)\f$ is of size
  \f$ (n-c) \times (n-c) \f$ which is far smaller than the \f$ (n+c) \times (n+c) \f$ 
  matrix used in the direct method. As it is relatively inexpensive, the
  dual variables are also evaluated
  \f[
    R \lambda = (Y^T FY)p_Y + (Y^T F Z)p_Z + Y^T g
  \f]
  and
  \f[
    \tau = S^T W S (x-\ddot{q})
  \f]


 \note Two modifications have been made to this implementation to bring the
       solution to \f$S \ddot{q}\f$ much closer to \f$S x\f$
       -# The vector \f$u^*\f$ has been modifed to \f$u^* = Sx + (S^T W^{-1} S)C\f$ so that the term \f$SC\f$ in the upper right hand side is compensated
       -# The weighting matrix \f$W\f$ has a main diagional that is scaled to be uniformly 100 times larger than the biggest element in M. This will drive the solution closer to \f$S x\f$ without hurting the scaling of the matrix too badly.

 \note The Lagrange multipliers are solved for and stored in the `force' field
       of the ConstraintSet structure.

 \note The sign of \f$\gamma\f$ in this documentation is consistent with RBDL
 and it is equal to \f$-1\f$ times the right hand side of the constraint
 expressed at the acceleration-level. It is more common to see \f$-\gamma\f$,
 in the literature and define \f$\gamma\f$ as the positive right-hand side of
 the acceleration equation.


 <b>References</b>
  -# Koch KH (2015). Using model-based optimal control for conceptional motion generation for the humannoid robot hrp-2 and design investigations for exo-skeletons. Heidelberg University (Doctoral dissertation).
  -# Kudruss M (under review as of May 2019). Nonlinear model-predictive control for the motion generation of humanoids. Heidelberg University  (Doctoral dissertation)

 \param model: rigid body model
 \param Q:     N-element vector of generalized positions
 \param QDot:  N-element vector of generalized velocities

 \param QDDotControls:
      N-element vector of generalized acceleration controls
     (\f$x\f$ in the above equation). If the idea of a control vector is
     unclear please read the above text for additional details.

 \param CS: Structure that contains information about the set of kinematic
            constraints. Note that the 'force' vector is appropriately updated
            after this function is called so that it contains the Lagrange
            multipliers.

 \param QDDotOutput:  N-element vector of generalized accelerations which
                      satisfy the kinematic constraints (\f$\ddot{q}\f$ in the
                      above equation)
 \param TauOutput: N-element vector of generalized forces which satisfy the
                   the equations of motion for this constrained system.                   
 \param f_ext External forces acting on the body in base coordinates
        (optional, defaults to NULL)

 */
RBDL_DLLAPI
void InverseDynamicsConstraintsRelaxed(
    Model &model,
    const Math::VectorNd &Q,
    const Math::VectorNd &QDot,
    const Math::VectorNd &QDDotControls,
    ConstraintSet &CS,
    Math::VectorNd &QDDotOutput,
    Math::VectorNd &TauOutput,
    std::vector<Math::SpatialVector> *f_ext  = NULL);

/**
 @brief An inverse-dynamics operator that can be applied to fully-actuated
        constrained systems.
  \par
  <b>Important</b>
  -# Set the actuated degrees-of-freedom using RigidBodyDynamics::ConstraintSet::SetActuationMap
  <b>prior</b> to calling this function.
  -# Use the function RigidBodyDynamics::isConstrainedSystemFullyActuated to
     determine if a system is fully actuated or not.
 \par
 This function implements an inverse-dynamics operator defined by Koch (1)
 (described in Eqn. 5.20) that can be applied to fully-actuated constraint
 systems and will solve for a set of physically-consistent
 \f$\ddot{q}\f$ and \f$\ddot{tau}\f$ given a desired \f$\ddot{q}^*\f$.
 \par
 To see test if a constrained system is fully actuated please use the function
 RigidBodyDynamics::isConstrainedSystemFullyActuated. If the constrained system
 is not fully actuated then the method
 RigidBodyDynamics::InverseDynamicsConstraintsRelaxed must be used instead.
 \par
 For a detailed explanation of the systems which cause this method to fail
 please read the text following Eqn. 5.20 in Koch's thesis, and the example that
 is given in Sec. 3 on pg 66. A brief summary is presented below.
 \par
 To begin,the generalized accelerations are partitioned into actuated
 parts \f$u\f$ and unactuated parts \f$v\f$
  \f[
  u = S \ddot{q}
  \f]
  where \f$S\f$ is an  \f$n_a\f$ (number of actuated degrees-of-freedom) by
  \f$n\f$ (number of degrees of freedom of the unconstrained system) selection
  matrix that picks out the actuated indices in \f$\ddot{q}\f$ and
  \f[
  v = P \ddot{q}
  \f]
  where \f$P\f$ is an  \f$n_u=n-n_a\f$ (number of unactuated degrees-of-freedom)
  by  \f$n\f$ selection matrix that picks out the unactuated indices in \ddot{q}.
  By construction
  \f[
    \left(\begin{array}{cc}
      PP^T & 0 \\
      0 & SS^T
    \end{array}\right) = I_{n}
  \f]
  and thus
  \f[
    \ddot{q} = S u + P v
  \f]
  We begin by projecting the constrained equations of motion
 \f[ \left( \begin{array}{cc}
      H & G^T \\
      G & 0
    \end{array} \right)
    \left( \begin{array}{c}
    \ddot{q} \\
    -\lambda
    \end{array} \right)
    =\left(
    \begin{array}{c}
       \tau - C\\
       \gamma
    \end{array}
    \right)
\f]
and adding the constraint that
\f[
 u - S \ddot{q}^* = 0
\f]
where \f$\ddot{q}^*\f$ is a vector of desired accelerations. By considering
only forces that are applied to the actuated parts (that is \f$S \tau\f$)
we can rearrange the system of equations
 \f[ \left( \begin{array}{ccc}
      H & G^T  & S^T\\
      G & 0 & 0 \\
      S & 0 & 0 \\
    \end{array} \right)
    \left( \begin{array}{c}
    \ddot{q} \\
    -\lambda \\
    -\tau
    \end{array} \right)
    =\left(
    \begin{array}{c}
       - C\\
       \gamma \\
       S\ddot{q}^*
    \end{array}
    \right)
\f]
By projecting this onto the onto the \f$S\f$ and \f$P\f$ spaces
 \f[
 \left(
   \begin{array}{cccc}
    S H S^T & S M P^T & S G^T & I \\
    P M S^T & P M P^T & P G^T & 0\\
    G S^T & G P^T & 0 & 0 \\
    I & 0 & 0 & 0 \\
   \end{array}
 \right)
 \left(
   \begin{array}{c}
    u \\
    v \\
    -\lambda\\
    -\tau
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     -SC\\
     -PC\\
    \gamma\\
    S \ddot{q}^*
   \end{array}
 \right)
 \f]
 it becomes clear that this system of equations will be singular if \f$GP^T\f$
 loses rank. Thus this method is appropriate to use provided that
  \f[
     \text{rank}( GP^T ) = n - n_a.
  \f]

 \note The implementation is currently a prototype method, and as such, can
       be greatly sped up. If you refer to Eqn. 5.20 in Koch's thesis the
       current implementation is solving the \f$(n+n_c+n_a)\times(n+n_c+n_a)\f$
       matrix directly. This block diagonal matrix can be solved in parts much
       faster.



 <b>References</b>
  -# Koch KH (2015). Using model-based optimal control for conceptional motion generation for the humannoid robot hrp-2 and design investigations for exo-skeletons. Heidelberg University (Doctoral dissertation).


 \param model: rigid body model
 \param Q:     N-element vector of generalized positions
 \param QDot:  N-element vector of generalized velocities
 \param QDDotDesired:  N-element vector of desired generalized accelerations
                       (\f$\ddot{q}^*\f$ in the above equation)
 \param CS: Structure that contains information about the set of kinematic
            constraints. Note that the 'force' vector is appropriately updated
            after this function is called so that it contains the Lagrange
            multipliers.

 \param QDDotOutput:  N-element vector of generalized accelerations which
                      satisfy the kinematic constraints (\f$\ddot{q}\f$ in the
                      above equation)
 \param TauOutput: N-element vector of generalized forces which satisfy the
                   the equations of motion for this constrained system.
 \param f_ext External forces acting on the body in base coordinates
        (optional, defaults to NULL)


*/

RBDL_DLLAPI
void InverseDynamicsConstraints(
    Model &model,
    const Math::VectorNd &Q,
    const Math::VectorNd &QDot,
    const Math::VectorNd &QDDotDesired,
    ConstraintSet &CS,
    Math::VectorNd &QDDotOutput,
    Math::VectorNd &TauOutput,
    std::vector<Math::SpatialVector> *f_ext  = NULL);

#ifndef RBDL_USE_SIMPLE_MATH
/**
  \brief A method to evaluate if the constrained system is fully actuated.

  \par 
   This method will evaluate the rank of \f$(GP^T)\f$
   in order to assess if the constrained system is fully
   actuated or is under actuated. If the system is fully actuated the
   exact method RigidBodyDynamics::InverseDynamicsConstraints can be used, otherwise only
   relaxed method RigidBodyDynamics::InverseDynamicsConstraintsRelaxed can be used.

  \note This method uses a relatively slow but accurate method to
        evaluate the rank.

 \param model: rigid body model
 \param Q:     N-element vector of generalized positions
 \param QDot:  N-element vector of generalized velocities
 \param CS: Structure that contains information about the set of kinematic
            constraints.
 \param f_ext External forces acting on the body in base coordinates
        (optional, defaults to NULL)
*/
RBDL_DLLAPI 
bool isConstrainedSystemFullyActuated(
    Model &model,
    const Math::VectorNd &Q,
    const Math::VectorNd &QDot,
    ConstraintSet &CS,
    std::vector<Math::SpatialVector> *f_ext  = NULL);
#endif

/** \brief Computes contact gain by constructing and solving the full lagrangian 
 *  equation
 *
 * This method builds and solves the linear system \f[
 \left(
   \begin{array}{cc}
     H & G^T \\
     G & 0
   \end{array}
 \right)
 \left(
   \begin{array}{c}
     \dot{q}^{+} \\
     \Lambda
   \end{array}
 \right)
 =
 \left(
   \begin{array}{c}
     H \dot{q}^{-} \\
    v^{+} 
   \end{array}
 \right)
 * \f] where \f$H\f$ is the joint space inertia matrix computed with the
 * CompositeRigidBodyAlgorithm(), \f$G\f$ are the point jacobians of the
 * contact points, \f$\dot{q}^{+}\f$ the generalized velocity after the
 * impact, \f$\Lambda\f$ the impulses at each constraint, \f$\dot{q}^{-}\f$
 * the generalized velocity before the impact, and \f$v^{+}\f$ the desired
 * velocity of each constraint after the impact (known beforehand, usually
 * 0). The value of \f$v^{+}\f$ can is specified via the variable
 * ConstraintSet::v_plus and defaults to 0.
 *
 * \note So far, only constraints acting along cartesian coordinate axes
 * are allowed (i.e. (1, 0, 0), (0, 1, 0), and (0, 0, 1)). Also, one must
 * not specify redundant constraints!
 * 
 * \par 
 *
 * \note To increase performance group constraints body and pointwise such
 * that constraints acting on the same body point are sequentially in
 * ConstraintSet. This can save computation of point Jacobians \f$G\f$.
 *
 * \param model rigid body model
 * \param Q     state vector of the internal joints
 * \param QDotMinus  velocity vector of the internal joints before the impact
 * \param CS the set of active constraints
 * \param QDotPlus velocities of the internals joints after the impact (output)
 */
RBDL_DLLAPI
void ComputeConstraintImpulsesDirect (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDotMinus,
  ConstraintSet &CS,
  Math::VectorNd &QDotPlus
);

/** \brief Resolves contact gain using SolveContactSystemRangeSpaceSparse()
 */
RBDL_DLLAPI
void ComputeConstraintImpulsesRangeSpaceSparse (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDotMinus,
  ConstraintSet &CS,
  Math::VectorNd &QDotPlus
);

/** \brief Resolves contact gain using SolveContactSystemNullSpace()
 */
RBDL_DLLAPI
void ComputeConstraintImpulsesNullSpace (
  Model &model,
  const Math::VectorNd &Q,
  const Math::VectorNd &QDotMinus,
  ConstraintSet &CS,
  Math::VectorNd &QDotPlus
);

/** \brief Solves the full contact system directly, i.e. simultaneously for 
 *  contact forces and joint accelerations.
 *
 * This solves a \f$ (n_\textit{dof} +
 * n_c) \times (n_\textit{dof} + n_c\f$ linear system.
 *
 * \param H the joint space inertia matrix
 * \param G the constraint jacobian
 * \param c the \f$ \mathbb{R}^{n_\textit{dof}}\f$ vector of the upper part of 
 * the right hand side of the system
 * \param gamma the \f$ \mathbb{R}^{n_c}\f$ vector of the lower part of the 
 * right hand side of the system
 * \param qddot result: joint accelerations
 * \param lambda result: constraint forces
 * \param A work-space for the matrix of the linear system 
 * \param b work-space for the right-hand-side of the linear system
 * \param x work-space for the solution of the linear system
 * \param type of solver that should be used to solve the system
 */
RBDL_DLLAPI
void SolveConstrainedSystemDirect (
  Math::MatrixNd &H, 
  const Math::MatrixNd &G, 
  const Math::VectorNd &c, 
  const Math::VectorNd &gamma, 
  Math::VectorNd &lambda, 
  Math::MatrixNd &A, 
  Math::VectorNd &b,
  Math::VectorNd &x,
  Math::LinearSolver &linear_solver
);

/** \brief Solves the contact system by first solving for the the joint 
 *  accelerations and then the contact forces using a sparse matrix 
 *  decomposition of the joint space inertia matrix.
 *
 * This method exploits the branch-induced sparsity by the structure
 * preserving \f$L^TL \f$ decomposition described in RBDL, Section 6.5.
 * 
 * \param H the joint space inertia matrix
 * \param G the constraint jacobian
 * \param c the \f$ \mathbb{R}^{n_\textit{dof}}\f$ vector of the upper part of 
 * the right hand side of the system
 * \param gamma the \f$ \mathbb{R}^{n_c}\f$ vector of the lower part of the 
 * right hand side of the system
 * \param qddot result: joint accelerations
 * \param lambda result: constraint forces
 * \param K work-space for the matrix of the constraint force linear system
 * \param a work-space for the right-hand-side of the constraint force linear 
 * system
 * \param linear_solver type of solver that should be used to solve the 
 * constraint force system
 */
RBDL_DLLAPI
void SolveConstrainedSystemRangeSpaceSparse (
  Model &model, 
  Math::MatrixNd &H, 
  const Math::MatrixNd &G, 
  const Math::VectorNd &c, 
  const Math::VectorNd &gamma, 
  Math::VectorNd &qddot, 
  Math::VectorNd &lambda, 
  Math::MatrixNd &K, 
  Math::VectorNd &a,
  Math::LinearSolver linear_solver
);

/** \brief Solves the contact system by first solving for the joint 
 *  accelerations and then for the constraint forces.
 *
 * This methods requires a \f$n_\textit{dof} \times n_\textit{dof}\f$
 * matrix of the form \f$\left[ \ Y \ | Z \ \right]\f$ with the property
 * \f$GZ = 0\f$ that can be computed using a QR decomposition (e.g. see
 * code for ForwardDynamicsContactsNullSpace()).
 *
 * \param H the joint space inertia matrix
 * \param G the constraint jacobian
 * \param c the \f$ \mathbb{R}^{n_\textit{dof}}\f$ vector of the upper part of 
 * the right hand side of the system
 * \param gamma the \f$ \mathbb{R}^{n_c}\f$ vector of the lower part of the 
 * right hand side of the system
 * \param qddot result: joint accelerations
 * \param lambda result: constraint forces
 * \param Y basis for the range-space of the constraints
 * \param Z basis for the null-space of the constraints
 * \param qddot_y work-space of size \f$\mathbb{R}^{n_\textit{dof}}\f$
 * \param qddot_z work-space of size \f$\mathbb{R}^{n_\textit{dof}}\f$
 * \param linear_solver type of solver that should be used to solve the system
 */
RBDL_DLLAPI
void SolveConstrainedSystemNullSpace (
  Math::MatrixNd &H, 
  const Math::MatrixNd &G, 
  const Math::VectorNd &c, 
  const Math::VectorNd &gamma, 
  Math::VectorNd &qddot, 
  Math::VectorNd &lambda,
  Math::MatrixNd &Y,
  Math::MatrixNd &Z,
  Math::VectorNd &qddot_y,
  Math::VectorNd &qddot_z,
  Math::LinearSolver &linear_solver
);


/** \brief Interface to define general-purpose constraints.
 *
* The CustomConstraint interface is a general-purpose interface that is rich
* enough to define time-varying constraints at the position-level \f$\phi_p(q,t)=0\f$,
* the velocity-level \f$\phi_v(\dot{q},t)=0\f$, or the acceleration-level
* \f$\phi_a(\ddot{q},t)=0\f$. These constraints all end up being applied at the
* acceleration-level by taking successive derivatives until we are left with
* \f$\Phi(\ddot{q},\dot{q},q,t)=0\f$
* The interface requires that the user populate
*
*   - G: \f$ \dfrac{ \partial \Phi(\ddot{q},\dot{q},q,t)}{ \partial \ddot{q}}\f$
*   - constraintAxis: the axis that the constraints are applied along
*   - gamma: \f$ \gamma = - ( \Phi(\ddot{q},\dot{q},q,t) - G \ddot{q})\f$
*   - errPos: The vector of \f$\phi_p(\dot{q},t)\f$. If the constraint is a velocity-level
*             constraint or higher this should be set to zero.
*   - errVel: The vector of \f$\phi_v(\dot{q},t)\f$. If the constraint is an acceleration-level
*             constraint this should be set to zero
*
* The matrix G and the vector gamma are required to compute accelerations that satisfy
* the desired constraints. The vectors errPos and errVel are required to apply Baumgarte
* stabilization (a constraint stabilization method) to stabilize the constraint. The
* interface provides (optional) interfaces for the functions
*
*   - CalcAssemblyPositionError
*   - CalcAssemblyPositionErrorJacobian
*   - CalcAssemblyVelocityError
*   - CalcAssemblyVelocityErrorJacobian
*
* These optional functions are used only during system assembly. This permits a velocity-level
* constraint (e.g. such as a rolling-without-slipping constraint) to define a position-level
* constraint to assemble the system (e.g. bring the rolling surfaces into contact with
* eachother). If you are defining a position-level constraint these optional functions
* should simply return errPos, G, errVel, and G respectively.
*
* It must be stated that this is an advanced feature of RBDL: you must have an in-depth
* knowledge of multibody-dynamics in order to write a custom constraint, and to write
* the corresponding test code to validate that the constraint is working. As a hint the
* test code in tests/CustomConstraintsTest.cc should contain a forward simulation of a simple
* system using the constraint and then check to see that system energy is conserved with only 
* a small error and the constraint is also satisfied with only a small error. The observed 
* error should drop as the integrator tolerances are lowered.
*/
struct RBDL_DLLAPI CustomConstraint {
    unsigned int mConstraintCount;
    //unsigned int mAssemblyConstraintCount;

    CustomConstraint(unsigned int constraintCount):mConstraintCount(constraintCount){}

    virtual ~CustomConstraint(){};    

    virtual void CalcConstraintsJacobianAndConstraintAxis(
                                          Model &model,
                                          unsigned int custom_constraint_id,
                                          const Math::VectorNd &Q,
                                          ConstraintSet &CS,
                                          Math::MatrixNd &G,
                                          unsigned int GrowStart,
                                          unsigned int GcolStart) = 0;

    virtual void CalcGamma( Model &model,
                            unsigned int custom_constraint_id,
                            const Math::VectorNd &Q,
                            const Math::VectorNd &QDot,
                            ConstraintSet &CS,
                            const Math::MatrixNd &Gblock,
                            Math::VectorNd &gamma,
                            unsigned int gammaStartIndex) = 0;


    virtual void CalcPositionError( Model &model,
                                    unsigned int custom_constraint_id,
                                    const Math::VectorNd &Q,
                                    ConstraintSet &CS,
                                    Math::VectorNd &err,
                                    unsigned int errStartIdx) = 0;

    virtual void CalcVelocityError( Model &model,
                                    unsigned int custom_constraint_id,
                                    const Math::VectorNd &Q,
                                    const Math::VectorNd &QDot,
                                    ConstraintSet &CS,
                                    const Math::MatrixNd &Gblock,
                                    Math::VectorNd &err,
                                    unsigned int errStartIndex) = 0;

};



/** @} */

} /* namespace RigidBodyDynamics */

/* RBDL_CONSTRAINTS_H */
#endif
