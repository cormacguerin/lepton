#include <iostream>
#include <fstream>
#include <string>
#include "neuron.h"
#include <math.h>

using namespace std;

namespace Neuron {

Neuron::Neuron()
{
}

Neuron::~Neuron()
{
}

/*
glm::vec3 Neuron::getPosition() {
	return this->position;
}

glm::quat Neuron::getRotation() {
	return this->rotation;
}

void Neuron::setPosition(glm::vec3 position) {
	this->position = position;
	for(std::vector<std::string>::iterator it = this->children.begin(); it != this->children.end(); ++it) {
		Render::Neuron* child = Render::NeuronManager::getInstance().getNeuron((*it));
		//glm::vec3 offset = child->position - this->position;
		child->position = inverse(this->rotation)*position;
	}
}

// this function does not work with child nodes.
void Neuron::setRotationByXYZ(float inX, float inY, float inZ) {

	x += inX/100;
	y += inY/100;
	z += inZ/100;

	glm::quat qX = normalize(glm::quat(cos(x/2), sin(x/2), 0, 0));
	glm::quat qY = normalize(glm::quat(cos(y/2), 0, sin(y/2), 0));
	glm::quat qZ = normalize(glm::quat(cos(z/2), 0, 0, sin(z/2)));

	// convert X to work space.
	glm::quat qXWorld = glm::inverse(glm::quat(1.0,0.0,0.0,0.0))*qX;

	glm::quat q = qXWorld*qZ*qY;

	this->rotation = q;

}

void Neuron::attachMesh(Render::Mesh mesh) {
	this->mesh = mesh;
}

void Neuron::setVisibility(bool v) {
	this->is_visible = v;
}

glm::mat4 Neuron::getModelViewMatrix() {
	glm::mat4 identityMatrix(1.0);
	glm::mat4 rotationMatrix = glm::mat4_cast(this->rotation);
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0), this->position );
	glm::mat4 viewMatrix = rotationMatrix * glm::inverse(translationMatrix);
	this->view_matrix = viewMatrix;
	return viewMatrix;
}

glm::mat4 Neuron::getModelViewProjectionMatrix(float fovY) {
	glm::mat4 projection = glm::perspective(fovY, 4.0f / 3.0f, 1.0f, 100.f);
	glm::mat4 modelView = getModelViewMatrix();
	return projection * modelView * glm::mat4(1.0f);
}
*/

}

