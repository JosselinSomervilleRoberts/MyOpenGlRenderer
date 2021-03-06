// ----------------------------------------------
// Polytechnique - INF584 "Image Synthesis"
//
// Base code for practical assignments.
//
// Copyright (C) 2022 Tamy Boubekeur
// All rights reserved.
// ----------------------------------------------
#define _USE_MATH_DEFINES

#include "Mesh.h"

#include <cmath>
#include <algorithm>
#define PI 3.14159f

using namespace std;

Mesh::~Mesh () {
	clear ();
}

BoundingBox Mesh::computeBoundingBox () const {
	BoundingBox bbox;
	for (size_t i = 0; i < m_vertexPositions.size (); i++) {
		if (i == 0)
			bbox.init (m_vertexPositions[i]);
		else
			bbox.extendTo (m_vertexPositions[i]);
	}
	return bbox;
}

void Mesh::computeBoundingSphere (glm::vec3 & center, float & radius) const {
	center = glm::vec3 (0.0);
	radius = 0.f;
	for (const auto & p : m_vertexPositions)
		center += p;
	center /= m_vertexPositions.size ();
	for (const auto & p : m_vertexPositions)
		radius = std::max (radius, distance (center, p));
}

void Mesh::recomputePerVertexNormals (bool angleBased) {
	m_vertexNormals.clear ();
	// Change the following code to compute a proper per-vertex normal
	m_vertexNormals.resize (m_vertexPositions.size (), glm::vec3 (0.0, 0.0, 0.0));
	for (auto & t : m_triangleIndices) {
		glm::vec3 e0 (m_vertexPositions[t[1]] - m_vertexPositions[t[0]]);
		glm::vec3 e1 (m_vertexPositions[t[2]] - m_vertexPositions[t[0]]);
		glm::vec3 n = normalize (cross (e0, e1));
		for (size_t i = 0; i < 3; i++)
			m_vertexNormals[t[i]] += n;
	}
	for (auto & n : m_vertexNormals)
		n = normalize (n);
}

void Mesh::clear () {
	m_vertexPositions.clear ();
	m_vertexTexCoords.clear ();
	m_vertexNormals.clear ();
	m_triangleIndices.clear ();
}


void Mesh::computePlanarParameterization() {
	m_vertexTexCoords.clear ();
	m_vertexTexCoords.resize (m_vertexPositions.size(), glm::vec2 (0.0, 0.0));

	glm::vec3 center;
	float radius;
	computeBoundingSphere(center, radius);

	for(size_t i=0; i<m_vertexPositions.size(); i++) {
		glm::vec2 onCircle = glm::vec2(0.5f * (m_vertexPositions[i] - center) / radius);
		float angle = std::atan2f(onCircle.y, onCircle.x);
		glm::vec2 finalPos = onCircle;
		if(((angle >= -PI/4.0f) && (angle <= PI/4.0f)) || (angle <= -3.0f*PI/4.0f) || (angle >= 3.0f*PI/4.0f))  onCircle /= pow(std::cos(angle), 2.0f);
		else  onCircle /= pow(std::sin(angle), 2.0f);
		m_vertexTexCoords[i] = glm::vec2(0.5f, 0.5f) + finalPos;
	}
}