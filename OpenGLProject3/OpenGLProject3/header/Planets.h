#ifndef PLANETS_H
#define PLANETS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <Sphere.h>

struct PlanetData {
	std::string name;
	float mass;           // in kg
	float radius;         // in meters - ADD THIS
	float distanceFromSun; // in meters
	float orbitalPeriod;   // in Earth years
	glm::vec3 color;        // RGB color for visualization
};

class Planet {
private:
	const float SCALE_FACTOR = 1e7f; // Scale factor to reduce planet sizes for visualization
public:
	std::vector<PlanetData> planets = {
		{"Sun", 1.989e30f, 6.96e8f, 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 0.0f)},          
		{"Mercury", 3.3011e23f, 2.4397e6f, 57.91e9f, 0.387f, glm::vec3(0.7f, 0.7f, 0.7f)}, 
		{"Venus", 4.8675e24f, 6.0518e6f, 108.21e9f, 0.723f, glm::vec3(0.9f, 0.7f, 0.5f)}, 
		{"Earth", 5.972e24f, 6.371e6f, 149.60e9f, 1.0f, glm::vec3(0.2f, 0.5f, 0.8f)},      
		{"Mars", 6.4171e23f, 3.3895e6f, 227.92e9f, 1.524f, glm::vec3(0.8f, 0.3f, 0.2f)},    
		{"Jupiter", 1.8982e27f, 6.9911e7f, 778.57e9f, 5.203f, glm::vec3(0.8f, 0.7f, 0.5f)},  
		{"Saturn", 5.6834e26f, 5.8232e7f, 1.4335e12f, 9.537f, glm::vec3(0.9f, 0.8f, 0.6f)},  
		{"Uranus", 8.6810e25f, 2.5362e7f, 2.8725e12f, 19.191f, glm::vec3(0.5f, 0.8f, 0.8f)}, 
		{"Neptune", 1.02413e26f, 2.4622e7f, 4.4951e12f, 30.07f, glm::vec3(0.3f, 0.4f, 0.9f)} 
	};

	Sphere sphere;
	PlanetData data;  // Correct - use the struct type

	// Add these member variables to store buffer IDs
	GLuint vaoId;
	GLuint vboId;
	GLuint iboId;


	// Constructor
	Planet(const std::string& name) : vaoId(0), vboId(0), iboId(0) {
		for (const auto& planet : planets) {
			if (planet.name == name) {
				data = planet;
				if (data.name == "Sun") {
					sphere = Sphere(data.radius / 2e8f, 36, 18, true); // Scale radius
				}
				else {
					sphere = Sphere(data.radius / SCALE_FACTOR, 36, 18, true); // Scale radius
				}
				
				planetSetup();
				return;
			}
		}
		// Default to Mercury if not found
		data = planets[0];
		sphere = Sphere(data.radius / SCALE_FACTOR, 36, 18, true);
		planetSetup();
	}

	// Get planet data by index
	PlanetData getPlanet(int index) {
		if (index >= 0 && index < planets.size())
			return planets[index];
		return planets[0]; // default to Mercury if index is out of range
	}

	// Destructor to clean up buffers
	~Planet() {
		glDeleteVertexArrays(1, &vaoId);
		glDeleteBuffers(1, &vboId);
		glDeleteBuffers(1, &iboId);
	}

	void planetSetup() {
		// create VAO to store all vertex array state to VAO
		glGenVertexArrays(1, &vaoId);
		glBindVertexArray(vaoId);

		// create VBO to copy interleaved vertex data (V/N/T) to VBO
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);           // for vertex data
		glBufferData(GL_ARRAY_BUFFER,                   // target
			sphere.getInterleavedVertexSize(), // data size, # of bytes
			sphere.getInterleavedVertices(),   // ptr to vertex data
			GL_STATIC_DRAW);                   // usage

		// create VBO to copy index data to VBO
		glGenBuffers(1, &iboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);   // for index data
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,           // target
			sphere.getIndexSize(),             // data size, # of bytes
			sphere.getIndices(),               // ptr to index data
			GL_STATIC_DRAW);                   // usage

		// activate attrib arrays
		glEnableVertexAttribArray(0);


		// set attrib arrays with stride and offset
		int stride = sphere.getInterleavedStride();     // should be 32 bytes
		glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);

		// unbind VAO and VBOs
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		//------------------------------------------------------------------------------------

	}

	glm::vec3 getColor() const {
		return data.color;
	}

	glm::vec3 getPosition() const {
		return glm::vec3(data.distanceFromSun / 1e10f, 0.0f, 0.0f);
	}

	// Draw the planet
	void draw(Shader& shader) {
		glBindVertexArray(vaoId);

		// Add color uniform
		glUniform3f(glGetUniformLocation(shader.ID, "ourColor"), data.color.r, data.color.g, data.color.b);

		// Draw with distance from sun as translation
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, getPosition());
		glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));


		glDrawElements(GL_TRIANGLES, sphere.getIndexCount(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	
};

#endif
