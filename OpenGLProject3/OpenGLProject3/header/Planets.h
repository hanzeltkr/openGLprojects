#ifndef PLANETS_H
#define PLANETS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <Sphere.h>
#include <iostream> // Include for logging
#include <Trail.h>

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
	const float SCALE_FACTOR = 5e7f;  // Even smaller planets
	glm::dvec3 velocity = glm::dvec3(0.0);  // double precision vectors
	glm::dvec3 position = glm::dvec3(0.0);

	const double G = 6.67430e-11;
	const double TIME_SCALE = 1000000.0;  // Much faster - was 100.0

	float lastUpdateTime = 0.0f;

	Trail* trail = nullptr;

public:
	std::vector<PlanetData> planets = {
		// Name,Mass (kg),Radius (m),Distance from Sun (m), Orbital Period (years), Color (RGB)
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
				

				// Initialize position and velocity
				if (data.name == "Sun") {
					position = glm::dvec3(0.0);  // CHANGE: vec3 → dvec3
					velocity = glm::dvec3(0.0);  // CHANGE: vec3 → dvec3
				}
				else {
					// Start at orbital distance (convert to scaled units for rendering)
					position = glm::dvec3(data.distanceFromSun / 1e10, 0.0, 0.0);  // CHANGE: vec3 → dvec3, remove 'f'
			
					// Calculate orbital velocity in real units: v = sqrt(GM/r)
					double sunMass = 1.989e30;  // CHANGE: float → double, remove 'f'
					double orbitalSpeed = glm::sqrt((G * sunMass) / data.distanceFromSun);  // CHANGE: float → double
			
					// Store velocity in scaled units (will be converted back in update)
					velocity = glm::dvec3(0.0, 0.0, orbitalSpeed / 1e10);  // CHANGE: vec3 → dvec3, remove 'f'
				}

				trail = new Trail(glm::vec3(position), 1.0f, 500, 0.2f); // Initialize trail


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

		if (trail) delete trail;
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
		return glm::vec3(position);  // Convert dvec3 to vec3
	}

	glm::vec3 getGravitationalForce(const Planet& sun) const {
		if (data.name == "Sun") return glm::vec3(0.0f);
		
		// Calculate distance vector (don't normalize yet!)
		glm::vec3 toSun = sun.position - this->position;
		float distance = glm::length(toSun);
		
		// Prevent division by zero
		if (distance < 1.0f) return glm::vec3(0.0f);
		
		// Normalize direction
		glm::vec3 direction = toSun / distance;
		
		// F = G * (m1 * m2) / r²
		// Since our distances are scaled by 1e10, we need to account for this
		float realDistance = distance * 1e10f;  // Convert back to meters for calculation
		float forceMagnitude = (G * data.mass * sun.data.mass) / (realDistance * realDistance);
		
		// Scale force back down for our simulation
		forceMagnitude /= 1e10f;
		
		return direction * forceMagnitude;
	}

	void update(float deltaTime, const Planet& sun) {
		if (data.name == "Sun") return;

		double dt = static_cast<double>(deltaTime) * TIME_SCALE;
		
		// Convert scaled position back to real position (meters)
		glm::dvec3 realPosition = position * 1e10;
		glm::dvec3 realSunPosition = sun.position * 1e10;
		
		// Calculate distance vector in meters
		glm::dvec3 toSun = realSunPosition - realPosition;
		double distance = glm::length(toSun);
		
		// DEBUG for Earth
		if (data.name == "Earth" && distance < 1e6) {
			std::cout << "ERROR: Distance too small: " << distance << std::endl;
			return;
		}
		
		if (distance < 1e6) return;
		
		// F = G * (m1 * m2) / r²
		glm::dvec3 direction = glm::normalize(toSun);
		double forceMagnitude = (G * static_cast<double>(data.mass) * static_cast<double>(sun.data.mass)) / (distance * distance);
		glm::dvec3 force = direction * forceMagnitude;
		
		// a = F / m
		glm::dvec3 acceleration = force / static_cast<double>(data.mass);
		
		// DEBUG for Earth
		if (data.name == "Earth") {
			std::cout << "Accel: " << acceleration.x << ", " << acceleration.y << ", " << acceleration.z << std::endl;
		}
	
		// Convert velocity to real units (meters/second)
		glm::dvec3 realVelocity = velocity * 1e10;
	
		// Update velocity and position in real units
		realVelocity += acceleration * dt;
		realPosition += realVelocity * dt;
	
		// Convert back to scaled units for rendering
		velocity = realVelocity / 1e10;
		position = realPosition / 1e10;


		if (trail) {
			trail->update(glm::vec3(position), 1.0f); // visibility = 1.0f
		
		}
	}

	// Draw the planet
	void draw(Shader& shader, const Planet& sun, float deltaTime) {

		update(deltaTime, sun);

		glBindVertexArray(vaoId);

		// Add color uniform
		glUniform3f(glGetUniformLocation(shader.ID, "ourColor"), data.color.r, data.color.g, data.color.b);

		// Draw with distance from sun as translation
		glm::mat4 model = glm::mat4(1.0f);
		glm::vec3 renderPos = glm::vec3(position);  // ADD THIS: Convert double to float for rendering
		model = glm::translate(model, renderPos);   // CHANGE: use renderPos instead of position
		glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, sphere.getIndexCount(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		if (trail) {
			trail->draw(shader, data.color);
		}
	}

	
};

#endif
