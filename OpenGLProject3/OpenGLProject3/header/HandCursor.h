#ifndef HANDCURSOR_H
#define HANDCURSOR_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <Shader.h>
#include <Camera.h>

#include <thread>
#include <atomic>
#include <cstdio>

class HandCursor {
private:
	std::vector<float> vertices;
	int segments;
	float radius = 1.0f;
	int vertexCount;

	unsigned int circleVAO, circleVBO;

	Shader* circleShader;
	float screenWidth;
	float screenHeight;

	bool key1WasPressed = false;
	float zoomStartLength = 0.0f;

	bool key2WasPressed = false;
	float initialYaw = 0.0f;
	float initialPitch = 0.0f;
	float initialHandYaw = 0.0f;
	float initialHandPitch = 0.0f;

	glm::vec3 orbitCenter = glm::vec3(0.0f, 0.0f, 0.0f);  // Point to orbit around
	float orbitDistance = 0.0f;

public:
	int x;
	int y;
	float l;
	int hx1, hy1, hx2, hy2;
	float hz1, hz2;
	float prevl;
	int prevX;
	int prevY;
	bool handExist;

	// For reading hand control data
	std::atomic<int> inputTypeCode; // Store input types as integer code
	std::atomic<int> handX;
	std::atomic<int> handY;
	std::atomic<int> x1, y1, x2, y2;
	std::atomic<float> length, z1, z2;
	std::atomic<bool> handDataReady;

	// Constructor
	HandCursor(Shader* shader, float screenW, float screenH, int seg = 30)
		: circleShader(shader),
		screenWidth(screenW),
		screenHeight(screenH),
		segments(seg),
		radius(0.1f),
		vertexCount(0),
		prevX(screenW / 2),
		prevY(screenH / 2),
		prevl(0),
		handExist(false),
		inputTypeCode(0),
		handX(0),
		handY(0),
		length(0),
		x1(0), y1(0), z1(0.0f),
		x2(0), y2(0), z2(0.0f),
		handDataReady(false) {
		
		// Create unit circle at (0,0)
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);

		for (int i = 0; i <= segments; i++) {
			float angle = 2.0f * 3.14159f * i / segments;
			vertices.push_back(radius * cos(angle));
			vertices.push_back(radius * sin(angle));
		}

		vertexCount = segments + 2;

		// Set up VAO VBO
		glGenVertexArrays(1, &circleVAO);
		glGenBuffers(1, &circleVBO);
		glBindVertexArray(circleVAO);
		glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// Start python thread
		std::thread handThread(&HandCursor::readHandData, this);
		handThread.detach();
	}

	
	void readHandData() {
		FILE* pipe = _popen("python C:\\Users\\namfa\\OneDrive\\Desktop\\Coding\\OpenGL01\\OpenGLProject3\\HandDetection1\\HandDetection1\\HandController.py", "r");

		if (!pipe) {
			std::cerr << "Failed to start Python script!" << std::endl;
			return;
		}

		char buffer[256];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
			int code, cx, cy, hx1, hy1, hx2, hy2;
			float l, hz1, hz2;
			if (sscanf_s(buffer, "%d %d %d %f %d %d %f %d %d %f", &code, &cx, &cy, &l, &hx1, &hy1, &hz1, &hx2, &hy2, &hz2) == 10) {
				l = roundf(l * 10.0f) / 10.0f;

				inputTypeCode.store(code);
				handX.store(cx);
				handY.store(cy);
				length.store(l);
				x1.store(hx1);
				y1.store(hy1);
				z1.store(hz1);
				x2.store(hx2);
				y2.store(hy2);
				z2.store(hz2);
				handDataReady.store(true);
			}
		}
		_pclose(pipe);
		
	}

	void updatePosition() {
		if (handDataReady.load()) {
			handExist = true;
			int method = inputTypeCode.load();
			x = handX.load();
			y = handY.load();
			l = length.load();
			hx1 = x1.load();
			hy1 = y1.load();
			hz1 = z1.load();
			hx2 = x2.load();
			hy2 = y2.load();
			hz2 = z2.load();

			const char* methodName = "unknown";
			if (method == 1) methodName = "position";
			else if (method == 2) methodName = "startHold";
			else if (method == 3) methodName = "holding";
			else if (method == 4) methodName = "endHold";
			else if (method == 5) methodName = "clicking";

			// For debugging
			std::cout << "Hand: " << methodName << ", " << x << ", " << y << ", " << l << ", " << hx1 << ", " << hy1 << ", " << hz1 << ", " << hx2 << ", " << hy2 << ", " << hz2 << std::endl;

			draw(x, y);

			prevX = x;
			prevY = y;

			handDataReady.store(false);
		}
		else {
			draw(prevX, prevY);
		}
	}

	void draw(float x, float y, float pixelRadius = 25.0f, glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f)) {
		circleShader->use();

		glUniform2f(glGetUniformLocation(circleShader->ID, "position"), float(x), float(y));
		glUniform2f(glGetUniformLocation(circleShader->ID, "screenSize"), 1280.0f, 720.0f);
		glUniform3f(glGetUniformLocation(circleShader->ID, "color"), 1.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(circleShader->ID, "radius"), 50.0f);

		glBindVertexArray(circleVAO);
		glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
		glBindVertexArray(0);
	}

	~HandCursor() {
		// Cleanup
		glDeleteVertexArrays(1, &circleVAO);
		glDeleteBuffers(1, &circleVBO);
	}
	

	void processInput(GLFWwindow* window, float deltaTime, Camera* camera) {

		bool key1Pressed = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;

		// Capture starting length
		if (key1Pressed && !key1WasPressed) {
			zoomStartLength = l;
		}

		if (key1Pressed) {
			float lengthDiff = l - zoomStartLength;
			float zoomSpeed = lengthDiff * 0.1f * deltaTime;
			camera->cameraPos -= zoomSpeed * camera->cameraFront;
		}

		key1WasPressed = key1Pressed;


		// === ORBITAL ROTATION (KEY_2) ===
		bool key2Pressed = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;

		// Initialize orbital parameters when KEY_2 is first pressed
		if (key2Pressed && !key2WasPressed) {
			// Set orbit center to origin
			orbitCenter = glm::vec3(0.0f, 0.0f, 0.0f);

			// Calculate distance from camera to orbit center
			orbitDistance = glm::length(orbitCenter - camera->cameraPos);

			// Store initial camera angles
			initialYaw = camera->yaw;
			initialPitch = camera->pitch;

			// Store initial hand orientation
			calculateRotationAngles(initialHandYaw, initialHandPitch);

			std::cout << "Orbit mode started - center: (" << orbitCenter.x << ", " 
					  << orbitCenter.y << ", " << orbitCenter.z << "), distance: " 
					  << orbitDistance << std::endl;
			std::cout << "Initial hand angles - yaw: " << initialHandYaw 
					  << ", pitch: " << initialHandPitch << std::endl;
		}

		if (key2Pressed) {
			// Calculate current hand orientation angles
			float currentHandYaw, currentHandPitch;
			calculateRotationAngles(currentHandYaw, currentHandPitch);

			// Calculate the difference from initial hand position
			float handYawDelta = currentHandYaw - initialHandYaw;
			float handPitchDelta = currentHandPitch - initialHandPitch;

			// Apply rotation offsets with deltaTime for smooth movement
			float sensitivity = 2.0f;  // Base sensitivity
			float rotationSpeed = sensitivity * deltaTime;  // Scale by deltaTime
			float newYaw = initialYaw + handYawDelta * rotationSpeed;
			float newPitch = initialPitch + handPitchDelta * rotationSpeed;

			// Clamp pitch
			if (newPitch > 89.0f) newPitch = 89.0f;
			if (newPitch < -89.0f) newPitch = -89.0f;

			// Calculate new camera position orbiting around center
			float yawRad = glm::radians(newYaw);
			float pitchRad = glm::radians(newPitch);

			camera->cameraPos.x = orbitCenter.x + orbitDistance * cos(pitchRad) * sin(yawRad);
			camera->cameraPos.y = orbitCenter.y + orbitDistance * sin(pitchRad);
			camera->cameraPos.z = orbitCenter.z + orbitDistance * cos(pitchRad) * cos(yawRad);

			camera->cameraFront = glm::normalize(orbitCenter - camera->cameraPos);

			// Update camera angles
			camera->yaw = newYaw;
			camera->pitch = newPitch;

			std::cout << "Orbit: yaw=" << newYaw << ", pitch=" << newPitch
				<< ", distance=" << orbitDistance << std::endl;
		}

		key2WasPressed = key2Pressed;
	}

	// Calculate rotation angles with Z scaling
	void calculateRotationAngles(float& outYaw, float& outPitch) {
		float centerX = (hx1 + hx2) / 2.0f;
		float centerY = (hy1 + hy2) / 2.0f;
		float centerZ = (hz1 + hz2) / 2.0f;

		float dx = hx1 - centerX;
		float dy = hy1 - centerY;
		float dz = hz1 - centerZ;

		// Scale up Z values
		float zScale = 500.0f; 
		float dzScaled = dz * zScale;

		outYaw = atan2(dx, dzScaled) * 180.0f / 3.14159f;
		float horizontalDist = sqrt(dx * dx + dzScaled * dzScaled);
		outPitch = atan2(dy, horizontalDist) * 180.0f / 3.14159f;

		// Debug output
		std::cout << "Hand tracking - dx: " << dx << ", dy: " << dy << ", dz: " << dz 
				  << " (scaled: " << dzScaled << ") -> yaw: " << outYaw << ", pitch: " << outPitch << std::endl;
	}

};


#endif
