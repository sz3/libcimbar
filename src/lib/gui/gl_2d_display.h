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
		glGenBuffers(3, _vbo.data());
		glGenVertexArrays(1, &_vao);
	}

	void clear()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void draw(GLuint texture)
	{
		GLuint prog = program();
		glUseProgram(prog);

		// Setup VBO
		glBindBuffer(GL_ARRAY_BUFFER, _vbo[_i]);
		glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), PLANE, GL_STATIC_DRAW);

		// Setup VAO
		GLint vertexPositionAttribute = glGetAttribLocation(prog, "vert");
		glBindVertexArray(_vao);
		glEnableVertexAttribArray(vertexPositionAttribute);
		glVertexAttribPointer(vertexPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Bind to texture
		GLuint textureUniform = glGetUniformLocation(prog, "tex");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(textureUniform, 0);

		// pass in rotation matrix
		GLuint rotateUniform = glGetUniformLocation(prog, "rot");
		std::array<GLfloat, 4> rot = rotation_matrix();
		glUniformMatrix2fv(rotateUniform, 1, false, rot.data());

		// pass in transform vector
		GLuint transformUniform = glGetUniformLocation(prog, "tform");
		std::pair<float, float> tform = shake_transform();
		glUniform2f(transformUniform, tform.first, tform.second);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Unbind
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		++_i;
		if (_i >= 3)
			_i = 0;
	}

	GLuint program() const
	{
		return *_p;
	}

	void rotate(unsigned i=1)
	{
		if (i == 0 or ++_rotation >= 4)
			_rotation = 0;
	}

	void shake(unsigned i=1)
	{
		if (i == 0 or ++_shake >= 8)
			_shake = 0;
	}

	std::array<GLfloat, 4> rotation_matrix() const
	{
		// just using sin and cos is probably better?
		switch (_rotation)
		{
			default:
			case 0:
				return {-1, 0, 0, 1};
			case 1: // right 180
				return {1, 0, 0, -1};
			case 2: // right 90
				return {0, 1, 1, 0};
			case 3: // right 270
				return {0, -1, -1, 0};
		}
	}

	std::pair<float, float> shake_transform() const
	{
		static constexpr std::array<std::pair<float, float>, 8> SHAKE_POS = {{
		    {0, 0}, {-.008, -.008}, {0, 0}, {.008, .008}, {0, 0}, {-.008, .008}, {0, 0}, {.008, -.008}
		}};
		return SHAKE_POS[_shake];
	}

protected:
	static std::shared_ptr<cimbar::gl_program> create()
	{
		/* rotations
		 *
		 * vec2 br = vec2(1.0f + vert.x, 1.0f - vert.y); // default
		 * vec2 bl = vec2(1.0f - vert.y, 1.0f - vert.x);
		 * vec2 tl = vec2(1.0f - vert.x, 1.0f + vert.y);
		 * vec2 tr = vec2(1.0f + vert.y, 1.0f + vert.x);
		*/
		static const std::string VERTEX_SHADER_SRC = R"(#version 300 es
		uniform mat2 rot;
		uniform vec2 tform;
		in vec4 vert;
		out vec2 texCoord;
		void main() {
		   gl_Position = vec4(vert.x, vert.y, 0.0f, 1.0f);
		   vec2 ori = vec2(vert.x, vert.y);
		   ori *= rot;
		   texCoord = vec2(1.0f - ori.x, 1.0f - ori.y) / 2.0;
		   texCoord += tform;
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
	std::array<GLuint, 3> _vbo;
	GLuint _vao;
	unsigned _i = 0;
	unsigned _rotation = 0;
	unsigned _shake = 0;
};

}
