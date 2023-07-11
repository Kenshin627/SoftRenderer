#include "PointLight.h"

PointLight::PointLight(const glm::vec3& position, const glm::vec3& color, float constant, float linear, float quadratic) :position(position), color(color), constant(constant), linear(linear), quadratic(quadratic){ }