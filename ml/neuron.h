/*
 * Author : Cormac Guerin 
 *
 * Name: object.h
 * A class defining simulation objects in our world.
 * 
 * For a robot for example an object would represent a limb etc.
 *
 * The position and orientation is stored in Eigen Matrices
 * The corresponding physics body is also store here
 *
 * Essentially this is a state container for the body/object
 *
 */

#ifndef _NEURON_H_
#define _NEURON_H_


#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "glm/ext.hpp"
#include <vector>

namespace Neuron {
	class Neuron {
		private:
		public:
			Neuron();
			~Neuron();

			std::vector<std::string> data;
			glm::vec3 angular_velocity;
//			glm::mat4 view_matrix;
//			glm::quat qX;
//			void setVisibility(bool v);
//			glm::quat getRotation();
	};
}

#endif

