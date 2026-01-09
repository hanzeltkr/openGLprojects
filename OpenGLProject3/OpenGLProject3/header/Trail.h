#ifndef TRAILS_H
#define TRAILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <Shader.h>
#include <glm/gtc/type_ptr.hpp>

inline float saturate(float x) {
    return glm::clamp(x, 0.0f, 1.0f);
}

class Trail {
public:

	// Segments initialized
	int segmentsUsed = 0;

	// Segments strips have
	int segments;

	// Segments length
	float segmentLength;

	// Coordinates of last segment end
	glm::vec3 lastSegmentPosition;

	// Fade out factor
	int fadeOutSegments = 100;
	float width = 1.0f;
	float minLength = 0.01f;




	void trailSetup() {
		// creat VAO to store all vertex array state
		glGenVertexArrays(1, &vaoId);
		glBindVertexArray(vaoId);

		// create VBO to copy vertex data
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);           // for vertex data
		glBufferData(GL_ARRAY_BUFFER, segments * 2 * sizeof(TrailVertex), NULL, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &iboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);   // for index data
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,           // target
			segments * 6 * sizeof(unsigned int),  // Indices are unsigned int, not vec3!
			NULL,                              // ptr to index data
			GL_DYNAMIC_DRAW);                  // usage

		// activate attrib arrays
		glEnableVertexAttribArray(0);

		// set attrib arrays with stride and offset
		int stride = sizeof(TrailVertex);               // should be 12 bytes
		glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);

		// TexCoord attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, stride, (void*)offsetof(TrailVertex, texCoord));

		// Visibility attribute
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 1, GL_FLOAT, false, stride, (void*)offsetof(TrailVertex, visibility));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}


	// Constructor
	GLuint vaoId;
	GLuint vboId;
	GLuint iboId;

	struct TrailVertex {
		glm::vec3 position;
		glm::vec2 texCoord;
		float visibility;
	};

	std::vector<TrailVertex> vertices;
	std::vector<unsigned int> indices;

	Trail(glm::vec3 startPosition, float segmentLength, int segments, float width)
		: vaoId(0), vboId(0), iboId(0), segmentLength(segmentLength), segments(segments), width(width) {

		lastSegmentPosition = startPosition;

		// Pre-allocate space for efficiency;
		indices.reserve(segments * 6);
		vertices.resize(segments * 2);

		trailSetup();
		fillIndexBuffer();

		// Upload indices to GPU (add this)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
			indices.size() * sizeof(unsigned int),
			indices.data());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// Destructor
	~Trail() {
		glDeleteVertexArrays(1, &vaoId);
		glDeleteBuffers(1, &vboId);
		glDeleteBuffers(1, &iboId);
	}

	void update(glm::vec3 newPosition, float visibility) {

		// Initialize the first segment, we have no indication for the direction
		// So displace 2 vertices to the left and right
		if (segmentsUsed == 0) {
			vertices[0] = { lastSegmentPosition + glm::vec3(-width, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), visibility };
			vertices[1] = { lastSegmentPosition + glm::vec3(width, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), visibility };
			segmentsUsed = 1;
		}

		glm::vec3 directionVector = newPosition - lastSegmentPosition;
		float directionLength = glm::length(directionVector);

		// If distance between newPosition and lastSegmentPosition exceeds segmentLength
		// Delete the oldest segment and add a new one at the other end
		if (directionLength > segmentLength) {
			glm::vec3 normalizedVector = directionVector / directionLength;

			// normal to direction
			// the trail always faces up, so we can use cross product with up vector
			glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f); // or pass from camera
			glm::vec3 normalVector = glm::normalize(glm::cross(normalizedVector, upVector));

			// How many segments are we in
			int currentSegment = segmentsUsed;

			// if we are already at max segments, remove the oldest
			if (currentSegment >= segments - 1) {
				shiftDownSegments();
				currentSegment = segments - 2;
			}
			else {
				segmentsUsed++;
			}

			// Update last segment with new position
			vertices[currentSegment * 2] = { newPosition + normalVector * width, glm::vec2(1.0f, 0.0f), 1.0f };
			vertices[currentSegment * 2 + 1] = { newPosition - normalVector * width, glm::vec2(1.0f, 1.0f), 1.0f };

			// Fade out
			// Cannot have moew fadeout segments than initial segments
			int max_fade_out_segments = std::min(fadeOutSegments, currentSegment);

			for(int i = 0; i < max_fade_out_segments; i++) {
				float visibilityTerm = std::min(1.0f / max_fade_out_segments * i, decodeVisibility(vertices[i * 2].visibility));
				visibilityTerm = encodeVisibility(visibilityTerm);
				
				// Apply to vertices
				vertices[i * 2].visibility = visibilityTerm;
				vertices[i * 2 + 1].visibility = visibilityTerm;
			}

			// Last segment's position is the current position now
			lastSegmentPosition = newPosition;
		}

		// If we are not further than a segment length but further than the minimum distance to change something
		// Save the last position and have minimum distance from that
		else if (directionLength > minLength) {
			glm::vec3 normalizedVector = directionVector / directionLength;
			glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f); // or pass from camera
			glm::vec3 normalVector = glm::normalize(glm::cross(normalizedVector, upVector));
			int currentSegment = segmentsUsed;

			vertices[currentSegment * 2] = { newPosition + normalVector * width, glm::vec2(1.0f, 0.0f), encodeVisibility(visibility) };
			vertices[currentSegment * 2 + 1] = { newPosition - normalVector * width, glm::vec2(1.0f, 1.0f), encodeVisibility(visibility) };

			// Adjust the orientation of the last vertices to have smooth trail
			if (currentSegment >= 2) {
				glm::vec3 directionVectorOld = vertices[(currentSegment - 1) * 2].position - 
                                                vertices[(currentSegment - 2) * 2].position;
				glm::vec3 normalVectorOld = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), directionVectorOld));
				normalVectorOld = normalVectorOld + (1.0f - saturate(glm::dot(normalVectorOld, normalVector))) * normalVector;
				normalVectorOld = glm::normalize(normalVectorOld);
				vertices[(currentSegment - 1) * 2].position = lastSegmentPosition + normalVectorOld * width;
				vertices[(currentSegment - 1) * 2 + 1].position = lastSegmentPosition - normalVectorOld * width;
			}

			// Visibility
			// Fade out the trail to the back
			int max_fade_out_segments = std::min(fadeOutSegments, segmentsUsed);

			for (int i = 0; i < max_fade_out_segments; i++) {
				// Fade from 0.0 (invisible) at start to 1.0 (visible) at fadeOutSegments
				float alpha = static_cast<float>(i) / static_cast<float>(max_fade_out_segments);

				vertices[i * 2].visibility = alpha;
				vertices[i * 2 + 1].visibility = alpha;
			}

			// ✅ Ensure remaining segments are fully visible
			for (int i = max_fade_out_segments; i < segmentsUsed; i++) {
				vertices[i * 2].visibility = 1.0f;
				vertices[i * 2 + 1].visibility = 1.0f;
			}

		}

		updateBuffers();
	}

	void fillIndexBuffer() {
		for (int i = 0; i < segments - 1; i++) {
			indices.push_back(0 + i * 2);
			indices.push_back(1 + i * 2);
			indices.push_back(2 + i * 2);
			indices.push_back(1 + i * 2);
			indices.push_back(3 + i * 2);
			indices.push_back(2 + i * 2);
		}
	}

	void updateBuffers() {
		if (vertices.empty() || indices.empty()) return;

		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, 
			vertices.size() * sizeof(TrailVertex), 
			vertices.data());

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 
			indices.size() * sizeof(unsigned int), 
			indices.data());

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void draw(Shader& shader, const glm::vec3& color) {
		if (indices.empty() || vertices.empty()) return;

		glBindVertexArray(vaoId);

		glUniform3f(glGetUniformLocation(shader.ID, "ourColor"), color.r, color.g, color.b);

		glm::mat4 model = glm::mat4(1.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		int indexCount = segmentsUsed * 6;  // Each segment = 2 triangles = 6 indices
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
	}

private:
	float encodeVisibility(float visibility) {
		return (visibility + 1) / 3.0f;
	}

	float decodeVisibility(float visibility) {
		return visibility * 3.0f - 1;
	}

	void shiftDownSegments() {
		for (int i = 0; i < (segments - 1); i++) {
			vertices[i * 2] = vertices[i * 2 + 2];
			vertices[i * 2 + 1] = vertices[i * 2 + 3];
		}
	}

};

#endif 