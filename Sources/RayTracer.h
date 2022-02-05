// ----------------------------------------------
// Polytechnique - INF584 "Image Synthesis"
//
// Base code for practical assignments.
//
// Copyright (C) 2022 Tamy Boubekeur
// All rights reserved.
// ----------------------------------------------
#pragma once

#include <random>
#include <cmath>
#include <algorithm>
#include <limits>
#include <memory>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Image.h"
#include "Scene.h"
#include "Ray.h"
#include "RayHit.h"
#include "Triangle.h"
#include "Material.h"


using namespace std;

class RayTracer {
public:
	
	RayTracer();
	virtual ~RayTracer();

	inline void setResolution (int width, int height) { m_imagePtr = make_shared<Image> (width, height); }
	inline std::shared_ptr<Image> image () { return m_imagePtr; }
	void init (const std::shared_ptr<Scene> scenePtr);
	void render (const std::shared_ptr<Scene> scenePtr);

	RayHit* rayIntersect(Ray& ray, Triangle& triangle);
	glm::vec3 shade(RayHit* rayHit, const std::shared_ptr<Scene> scenePtr);
	glm::vec3 get_fd(Material& material);
	glm::vec3 get_fs(Material& material, glm::vec3 w0, glm::vec3 wi, glm::vec3 wh, glm::vec3 n);
	glm::vec3 get_r(Material& material, glm::vec3 fPosition, glm::vec3 fNormal, glm::vec3 lightDirection, float lightIntensity, glm::vec3 lightColor);

private:
	std::shared_ptr<Image> m_imagePtr;
};