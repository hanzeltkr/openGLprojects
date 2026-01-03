#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Camera {
public:
	// Camera setup
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Mouse position
	float lastX = 400.0f, lastY = 300.0f;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov = 45.0f;
	bool firstMouse = true;
	bool leftMousePressed = false;
	float mouseSensitivity = 0.1f;

	// Function declarations
	void processInput(GLFWwindow* window, float deltaTime) {
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		float cameraSpeed = 15.0f * deltaTime;

		// for moving camera
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += cameraSpeed * cameraFront;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= cameraSpeed * cameraFront;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			cameraPos += cameraSpeed * cameraUp;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			cameraPos -= cameraSpeed * cameraUp;
	}

	void mouse_callback(GLFWwindow* window, double xpos, double ypos) {

		// Makes the camera not jump when the window receives mouse cursor
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}
		// Only update camera if left mouse button is pressed
		if (!leftMousePressed)
			return;

		// Calculate offset movement between last and current frame
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		xoffset *= mouseSensitivity;  // Change from const float sensitivity = 0.1f;
		yoffset *= mouseSensitivity;

		// Add offset values to pitch and yaw
		yaw += xoffset;
		pitch += yoffset;

		// Makes LookAt not flip if look higher than 90 degrees
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		// Calculate actual direction vector
		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(direction);


	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				leftMousePressed = true;
				firstMouse = true;
			}
			else if (action == GLFW_RELEASE)
				leftMousePressed = false;
		}
	}

	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			mouseSensitivity += (float)yoffset * 0.01f;
			if (mouseSensitivity < 0.01f)
				mouseSensitivity = 0.01f;
			if (mouseSensitivity > 1.0f)
				mouseSensitivity = 1.0f;
		}
		else {
			fov -= (float)yoffset;
			if (fov < 1.0f)
				fov = 1.0f;
			if (fov > 45.0f)
				fov = 45.0f;
		}
	}

};

#endif