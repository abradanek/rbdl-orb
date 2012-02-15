/*
 * RBDL - Rigid Body Library
 * Copyright (c) 2011 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the zlib license. See LICENSE for more details.
 */

#include <iostream>
#include <limits>
#include <assert.h>

#include "mathutils.h"
#include "Logging.h"

#include "Model.h"
#include "Body.h"
#include "Joint.h"

using namespace RigidBodyDynamics;
using namespace RigidBodyDynamics::Math;

void Model::Init() {
	experimental_floating_base = false;

	Body root_body;
	Joint root_joint;

	Vector3d zero_position (0., 0., 0.);
	SpatialVector zero_spatial (0., 0., 0., 0., 0., 0.);

	// structural information
	lambda.push_back(0.);
	mu.push_back(std::vector<unsigned int>());
	dof_count = 0;

	gravity = Vector3d (0., -9.81, 0.);

	// state information
	q = VectorNd::Zero(1);
	qdot = VectorNd::Zero(1);
	qddot = VectorNd::Zero(1);
	tau = VectorNd::Zero(1);

	v.push_back(zero_spatial);
	a.push_back(zero_spatial);

	// Joints
	mJoints.push_back(root_joint);
	S.push_back (zero_spatial);
	X_T.push_back(SpatialTransform());
	
	// Dynamic variables
	c.push_back(zero_spatial);
	IA.push_back(SpatialMatrixIdentity);
	pA.push_back(zero_spatial);
	U.push_back(zero_spatial);

	u = VectorNd::Zero(1);
	d = VectorNd::Zero(1);

	f.push_back (zero_spatial);
	Ic.push_back (SpatialMatrixIdentity);

	// Bodies
	X_lambda.push_back(SpatialTransform());
	X_base.push_back(SpatialTransform());

	mBodies.push_back(root_body);
	mBodyNames.push_back("ROOT");
}

unsigned int Model::AddBody (const unsigned int parent_id,
		const SpatialTransform &joint_frame,
		const Joint &joint,
		const Body &body,
		std::string body_name) {
	assert (lambda.size() > 0);
	assert (joint.mJointType != JointTypeUndefined);

	// structural information
	lambda.push_back(parent_id);
	mu.push_back(std::vector<unsigned int>());
	mu.at(parent_id).push_back(q.size());

	dof_count += 1;

	// state information
	q.resize (dof_count + 1);
	qdot.resize (dof_count + 1);
	qddot.resize (dof_count + 1);
	tau.resize (dof_count + 1);

	v.push_back(SpatialVector(0., 0., 0., 0., 0., 0.));
	a.push_back(SpatialVector(0., 0., 0., 0., 0., 0.));

	// Joints
	mJoints.push_back(joint);
	S.push_back (joint.mJointAxis);
	// we have to invert the transformation as it is later always used from the
	// child bodies perspective.
	X_T.push_back(joint_frame);

	// Dynamic variables
	c.push_back(SpatialVector(0., 0., 0., 0., 0., 0.));
	IA.push_back(body.mSpatialInertia);
	pA.push_back(SpatialVector(0., 0., 0., 0., 0., 0.));
	U.push_back(SpatialVector(0., 0., 0., 0., 0., 0.));

	d = VectorNd::Zero (dof_count + 1);
	u = VectorNd::Zero (dof_count + 1);

	f.push_back (SpatialVector (0., 0., 0., 0., 0., 0.));
	Ic.push_back (SpatialMatrixIdentity);

	// Bodies
	X_lambda.push_back(SpatialTransform());
	X_base.push_back(SpatialTransform());
	mBodies.push_back(body);
	mBodyNames.push_back(body_name);

	return q.size() - 1;
}

unsigned int Model::SetFloatingBaseBody (const Body &body) {
	assert (lambda.size() >= 0);

	if (experimental_floating_base) {
		// we also will have 6 more degrees of freedom
		dof_count += 6;

		q.resize (dof_count + 1);
		qdot.resize (dof_count + 1);
		qddot.resize (dof_count + 1);
		tau.resize (dof_count + 1);

		// parent is the maximum possible value to mark it as having no parent
		lambda.at(0) = std::numeric_limits<unsigned int>::max();

		// Bodies
		X_lambda.at(0) = SpatialTransform();
		X_base.at(0) = SpatialTransform();
		mBodies.at(0) = body;
		return 0;
	} else {
		// Add five zero weight bodies and append the given body last. Order of
		// the degrees of freedom is:
		// 		tx ty tz rz ry rx
		//

		unsigned body_tx_id;
		Body body_tx (0., Vector3d (0., 0., 0.), Vector3d (0., 0., 0.));
		Joint joint_tx (JointTypePrismatic, Vector3d (1., 0., 0.));
		body_tx_id = this->AddBody(0, Xtrans (Vector3d (0., 0., 0.)), joint_tx, body_tx);

		unsigned body_ty_id;
		Body body_ty (0., Vector3d (0., 0., 0.), Vector3d (0., 0., 0.));
		Joint joint_ty (JointTypePrismatic, Vector3d (0., 1., 0.));
		body_ty_id = this->AddBody(body_tx_id, Xtrans (Vector3d (0., 0., 0.)), joint_ty, body_ty);

		unsigned body_tz_id;
		Body body_tz (0., Vector3d (0., 0., 0.), Vector3d (0., 0., 0.));
		Joint joint_tz (JointTypePrismatic, Vector3d (0., 0., 1.));
		body_tz_id = this->AddBody(body_ty_id, Xtrans (Vector3d (0., 0., 0.)), joint_tz, body_tz);

		unsigned body_rz_id;
		Body body_rz (0., Vector3d (0., 0., 0.), Vector3d (0., 0., 0.));
		Joint joint_rz (JointTypeRevolute, Vector3d (0., 0., 1.));
		body_rz_id = this->AddBody(body_tz_id, Xtrans (Vector3d (0., 0., 0.)), joint_rz, body_rz);

		unsigned body_ry_id;
		Body body_ry (0., Vector3d (0., 0., 0.), Vector3d (0., 0., 0.));
		Joint joint_ry (JointTypeRevolute, Vector3d (0., 1., 0.));
		body_ry_id = this->AddBody(body_rz_id, Xtrans (Vector3d (0., 0., 0.)), joint_ry, body_ry);

		unsigned body_rx_id;
		Joint joint_rx (JointTypeRevolute, Vector3d (1., 0., 0.));
		body_rx_id = this->AddBody(body_ry_id, Xtrans (Vector3d (0., 0., 0.)), joint_rx, body);

		return body_rx_id;
	}
}

unsigned int Model::GetBodyId (const char *id) {
	for (unsigned int i = 0; i < mBodyNames.size(); i++) {
		if (mBodyNames[i] == id)
			return i;
	}

	return std::numeric_limits<unsigned int>::max();
}
