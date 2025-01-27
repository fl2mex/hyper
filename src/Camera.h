#pragma once
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

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

		void ProcessInput(GLFWwindow* window, UserActions userActions, float cameraSpeed, float mouseSensitivity)
		{
			static glm::vec3 cameraZ = glm::vec3(0.0f, 0.0f, 1.0f);
			static glm::vec3 cameraX = glm::vec3(1.0f, 0.0f, 0.0f);
			static glm::vec3 worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);

			ProcessKeyboardInput(userActions, cameraSpeed, cameraZ, cameraX, worldUp);
			ProcessMouseInput(window, userActions, mouseSensitivity, cameraZ, cameraX, worldUp);

			cameraX = glm::normalize(glm::vec3{
				cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
				sin(glm::radians(pitch)),
				sin(glm::radians(yaw)) * cos(glm::radians(pitch)) });
			cameraZ = glm::normalize(glm::cross(cameraX, glm::vec3(0.0f, 1.0f, 0.0f)));
			worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);
		}

		void ProcessKeyboardInput(UserActions userActions, float cameraSpeed, glm::vec3 cameraZ, glm::vec3 cameraX, glm::vec3 worldUp)
		{
			if (ImGui::GetIO().WantCaptureKeyboard)
				return;

			velocity = glm::vec3(0.0f);
			float speed = cameraSpeed;
			if (userActions.Keys[GLFW_KEY_LEFT_CONTROL])
				speed *= 2;
			if (userActions.Keys[GLFW_KEY_W])
				velocity += cameraZ * -speed;
			if (userActions.Keys[GLFW_KEY_S])
				velocity += cameraZ * speed;
			if (userActions.Keys[GLFW_KEY_A])
				velocity += cameraX * -speed;
			if (userActions.Keys[GLFW_KEY_D])
				velocity += cameraX * speed;
			if (userActions.Keys[GLFW_KEY_Q])
				velocity += worldUp * speed;
			if (userActions.Keys[GLFW_KEY_E])
				velocity += worldUp * -speed;
		}

		void ProcessMouseInput(GLFWwindow* window, UserActions userActions, float mouseSensitivity, glm::vec3 cameraZ, glm::vec3 cameraX, glm::vec3 worldUp)
		{
			if (ImGui::GetIO().WantCaptureMouse)
				return;

			static glm::vec2 lastMousePos;
			float xOffset = userActions.MousePos[0] - lastMousePos.x;
			float yOffset = lastMousePos.y - userActions.MousePos[1];
			lastMousePos = glm::vec2(userActions.MousePos[0], userActions.MousePos[1]);
			if (userActions.MouseButtons[GLFW_MOUSE_BUTTON_LEFT])
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				yaw += xOffset * mouseSensitivity;
				pitch += yOffset * mouseSensitivity;
				pitch = glm::clamp(pitch, -glm::half_pi<float>(), glm::half_pi<float>());
			}
			else
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

	};
}