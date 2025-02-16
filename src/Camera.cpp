#include "Camera.h"

hyper::UserActions userActions{};

namespace hyper
{
	void Camera::ProcessInput(GLFWwindow* window, float cameraSpeed, float mouseSensitivity)
	{
		static glm::vec3 cameraZ = glm::vec3(0.0f, 0.0f, 1.0f);									 // Crossing what used to be cameraX would move the
		static glm::vec3 worldRight = glm::inverse(GetRotationMatrix()) * glm::vec4(1, 0, 0, 0); // camera in the y direction which I don't want, so
		static glm::vec3 worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);	 // we only do it moving via forwards/backwards or Q/E

		ProcessKeyboardInput(cameraSpeed, cameraZ, worldUp, worldRight);
		ProcessMouseInput(window, mouseSensitivity);

		cameraZ = glm::normalize(glm::vec3{
			sin(-glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			});
		worldUp = glm::inverse(GetRotationMatrix()) * glm::vec4(0, 1, 0, 0);
	}

	void Camera::ProcessKeyboardInput(float cameraSpeed, glm::vec3 cameraZ, glm::vec3 worldUp, glm::vec3 worldRight)
	{
		velocity = glm::vec3(0.0f);
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;

		float speed = cameraSpeed;
		//if (userActions.Keys[GLFW_KEY_LEFT_CONTROL]) // Can't switch statement it to look nice :(
		//	speed *= 2;
		//if (userActions.Keys[GLFW_KEY_W])
		//	velocity += cameraZ * -speed;
		//if (userActions.Keys[GLFW_KEY_S])
		//	velocity += cameraZ * speed;
		//if (userActions.Keys[GLFW_KEY_A])
		//	velocity += worldRight * -speed;
		//if (userActions.Keys[GLFW_KEY_D])
		//	velocity += worldRight * speed;
		//if (userActions.Keys[GLFW_KEY_Q])
		//	velocity += worldUp * speed;
		//if (userActions.Keys[GLFW_KEY_E])
		//	velocity += worldUp * -speed;
		//if ((userActions.Keys[GLFW_KEY_SPACE] & KeyHeld) == 0)
		//{
		//	if (userActions.Keys[GLFW_KEY_SPACE] & KeyDown)
		//	{
		//		userActions.Keys[GLFW_KEY_SPACE] | KeyHeld;
		//		std::cout << "IT HAPPENEDDDD: " << wtfCounter << std::endl;
		//		wtfCounter++;
		//	}
		//}
		//else
		//	userActions.Keys[GLFW_KEY_SPACE] ^ KeyHeld;

		// working but want to change
		//
		//static bool held = false;
		// 
		//if (!held)
		//{
		//	if (userActions.Keys[GLFW_KEY_SPACE].KeyState) // Check if Space is pressed
		//	{
		//		Logger::logger->Log("KeyDown");
		//		held = true;
		//	}
		//}
		//else if (!userActions.Keys[GLFW_KEY_SPACE].KeyState) // Check if Space is not pressed IF KeyHeld is true
		//{
		//	Logger::logger->Log("KeyUp");
		//	held = false;
		//}

		// not working
		if (!userActions.Keys[GLFW_KEY_SPACE].KeyHeld)
		{
			if (userActions.Keys[GLFW_KEY_SPACE].KeyState)
			{
				Logger::logger->Log("KeyDown");
				userActions.Keys[GLFW_KEY_SPACE].KeyHeld = true;
			}
		}
		else if (!userActions.Keys[GLFW_KEY_SPACE].KeyState)
		{
			Logger::logger->Log("KeyUp");
			userActions.Keys[GLFW_KEY_SPACE].KeyHeld = false;
		}
	}
	
	void Camera::ProcessMouseInput(GLFWwindow* window, float mouseSensitivity)
	{
		if (ImGui::GetIO().WantCaptureMouse) // Seperate so you can hover over window while still maintaining movement control
			return;							 // and also vice versa with using the window and not moving the camera's pitch/yaw

		static glm::vec2 lastMousePos;
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
}