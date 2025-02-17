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

		glm::vec3 cameraZ = glm::vec3(0.0f, 0.0f, 1.0f);									 // Crossing what used to be cameraX would move the
		glm::vec3 worldRight = glm::inverse(GetRotationMatrix()) * glm::vec4(1, 0, 0, 0); // camera in the y direction which I don't want, so
		glm::vec3 worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);	 // we only do it moving via forwards/backwards or Q/E

		glm::vec2 lastMousePos;

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

		// Need to keep these three functions below in the header or else it breaks?? Can't make a .cpp file
		void ProcessInput(GLFWwindow* window, float cameraSpeed, float mouseSensitivity)
		{
			ProcessKeyboardInput(cameraSpeed, cameraZ, worldUp, worldRight);
			ProcessMouseInput(window, mouseSensitivity);
			cameraZ = glm::normalize(glm::vec3{
				sin(-glm::radians(yaw)) * cos(glm::radians(pitch)),
				sin(glm::radians(pitch)),
				cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
				});
			worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);
		}

		void ProcessKeyboardInput(float cameraSpeed, glm::vec3 cameraZ, glm::vec3 worldUp, glm::vec3 worldRight)
		{
			velocity = glm::vec3(0.0f);
			if (ImGui::GetIO().WantCaptureKeyboard)
				return;

			float speed = cameraSpeed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_LEFT_CONTROL])) // Can't switch statement it to look nice :(
				speed *= 2;
			if (KeyCheck(userActions.Keys[GLFW_KEY_W]))
				velocity += cameraZ * -speed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_S]))
				velocity += cameraZ * speed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_A]))
				velocity += worldRight * -speed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_D]))
				velocity += worldRight * speed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_Q]))
				velocity += worldUp * speed;
			if (KeyCheck(userActions.Keys[GLFW_KEY_E]))
				velocity += worldUp * -speed;
			if (KeyClick(userActions.Keys[GLFW_KEY_SPACE]))
				Logger::logger->Log("Spacebar pressed");
		}

		void ProcessMouseInput(GLFWwindow* window, float mouseSensitivity)
		{
			if (ImGui::GetIO().WantCaptureMouse) // Seperate so you can hover over window while still maintaining movement control
				return;							 // and also vice versa with using the window and not moving the camera's pitch/yaw

			float xOffset = userActions.MousePos[0] - lastMousePos.x;
			float yOffset = lastMousePos.y - userActions.MousePos[1];
			lastMousePos = glm::vec2(static_cast<float>(userActions.MousePos[0]), static_cast<float>(userActions.MousePos[1]));
			if (userActions.MouseButtons[GLFW_MOUSE_BUTTON_LEFT])
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				yaw += xOffset * mouseSensitivity;
				pitch += yOffset * mouseSensitivity;
				pitch = glm::clamp(pitch, -glm::half_pi<float>(), glm::half_pi<float>());
			}
			else // ugly code <3
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	};
}