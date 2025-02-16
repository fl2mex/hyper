#pragma once
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

#include "Logger.h"
#include "UserActions.h"

namespace hyper
{
	struct Camera
	{
		glm::vec3 velocity{ 0.0f };
		glm::vec3 position{ 0.0f, 0.0f, 5.0f };

		float pitch{ 0.0f };
		float yaw{ 0.0f };

		glm::mat4 GetViewMatrix() const
		{
			glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
			glm::mat4 cameraRotation = GetRotationMatrix();
			return glm::inverse(cameraTranslation * cameraRotation);
		}

		glm::mat4 GetRotationMatrix() const
		{
			glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
			glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });
			return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
		}

		void Update(float deltaTime)
		{
			glm::mat4 cameraRotation = GetRotationMatrix();
			position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.0f)) * deltaTime;
		}

		void ProcessKeyboardInput(float cameraSpeed, glm::vec3 cameraZ, glm::vec3 worldUp, glm::vec3 worldRight);

		void ProcessMouseInput(GLFWwindow* window, float mouseSensitivity);

		void ProcessInput(GLFWwindow* window, float cameraSpeed, float mouseSensitivity);
	};
}