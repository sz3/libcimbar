/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "gl_program.h"
#include "gl_shader.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <memory>

namespace cimbar {

class gl_2d_display
{
protected:
	static constexpr GLfloat PLANE[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};

public:
	gl_2d_display()
		: _p(create())
	{
	}

	void draw(GLuint texture)
	{
		GLuint prog = program();
		glUseProgram(prog);

		// Setup VBO
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), PLANE, GL_STATIC_DRAW);

		// Setup VAO
		GLuint vao;
		glGenVertexArrays(1, &vao);
		GLint vertexPositionAttribute = glGetAttribLocation(prog, "vert");
		glBindVertexArray(vao);
		glEnableVertexAttribArray(vertexPositionAttribute);
		glVertexAttribPointer(vertexPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Bind to texture
		GLuint textureUniform = glGetUniformLocation(prog, "tex");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(textureUniform, 0);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Unbind
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	GLuint program() const
	{
		return *_p;
	}

protected:
	static std::shared_ptr<cimbar::gl_program> create()
	{
		static const std::string VERTEX_SHADER_SRC = R"(#version 300 es
		in vec4 vert;
		out vec2 texCoord;
		void main() {
		   gl_Position = vec4(vert.x, vert.y, 0.0f, 1.0f);
		   texCoord = vec2((vert.x + 1.0f) / 2.0f, (1.0f - vert.y) / 2.0);
		})";

		static const std::string FRAGMENT_SHADER_SRC = R"(#version 300 es
		precision mediump float;
		uniform sampler2D tex;
		in vec2 texCoord;
		out vec4 finalColor;
		void main() {
		   finalColor = texture(tex, texCoord);
		})";

		GLuint vertexShader = cimbar::gl_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
		GLuint fragmentShader = cimbar::gl_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);
		return std::make_shared<cimbar::gl_program>(vertexShader, fragmentShader, "vert");
	}

protected:
	std::shared_ptr<cimbar::gl_program> _p;
};

}
