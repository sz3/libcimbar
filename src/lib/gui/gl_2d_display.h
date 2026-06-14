/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "gl_program.h"
#include "gl_shader.h"
#include "mat_to_gl.h"
#include "util/loop_iterator.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <memory>

namespace cimbar {

class gl_2d_display_program
{
protected:
	static constexpr const char* VERTEX_SHADER_SRC = R"(#version 300 es
	uniform mat2 rot;
	uniform vec2 tform;
	in vec4 vert;
	out vec2 texCoord;
	void main() {
	   gl_Position = vec4(vert.x, vert.y, 0.0f, 1.0f);
	   vec2 ori = vec2(vert.x, vert.y);
	   ori *= rot;
	   texCoord = vec2(1.0f - ori.x, 1.0f - ori.y) / 2.0;
	   texCoord -= tform;
	})";

	static constexpr const char* FRAGMENT_SHADER_SRC = R"(#version 300 es
	precision mediump float;
	uniform sampler2D tex;
	in vec2 texCoord;
	out vec4 finalColor;
	void main() {
	   finalColor = texture(tex, texCoord);
	})";

public:
	gl_2d_display_program()
	{
		GLuint vertexShader = cimbar::gl_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
		GLuint fragmentShader = cimbar::gl_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);

		_p = std::make_unique<cimbar::gl_program>(
			vertexShader, fragmentShader, "vert"
		);
	}

	GLuint id() const
	{
		if (!_p)
			return 0;
		return (GLuint)*_p;
	}

	GLint uniform_tex()
	{
		if (_texLoc < 0 and id() > 0)
			_texLoc = glGetUniformLocation(id(), "tex");
		return _texLoc;
	}

	GLint uniform_rot()
	{
		if (_rotLoc < 0 and id() > 0)
			_rotLoc = glGetUniformLocation(id(), "rot");
		return _rotLoc;
	}

	GLint uniform_tform()
	{
		if (_tformLoc < 0 and id() > 0)
			_tformLoc = glGetUniformLocation(id(), "tform");
		return _tformLoc;
	}

protected:
	std::unique_ptr<cimbar::gl_program> _p;
	GLint  _texLoc = -1;
	GLint  _rotLoc = -1;
	GLint  _tformLoc = -1;
};

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

	// just using sin and cos is probably better?
	static constexpr std::array<std::array<GLfloat, 4>, 4> ROTATIONS = {{
	    {-1, 0, 0, 1},
	    {1, 0, 0, -1}, // right 180
	    {0, 1, 1, 0},  // right 90
	    {0, -1, -1, 0} // right 270
	}};

	static std::array<std::pair<GLfloat, GLfloat>, 4> computeShakePos(float dim)
	{
		float shake = 8.0f / dim; // 1080
		float zero = 0.0f;
		return {{
			{zero, zero},
			{zero-shake, zero-shake},
			{zero, zero},
			{zero+shake, zero+shake}
		}};
	}

public:
	gl_2d_display(float dim)
	    : _shakePos(computeShakePos(dim))
	    , _shake(_shakePos)
	    , _rotation(ROTATIONS)
	{
		init();
	}

	~gl_2d_display()
	{
		if (_texid)
			glDeleteTextures(1, &_texid);
		if (_vbo)
			glDeleteBuffers(1, &_vbo);
		if (_vao)
			glDeleteVertexArrays(1, &_vao);
	}

	bool good() const
	{
		return _p.id() != 0;
	}

	void clear()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	bool load(const cv::Mat& img)
	{
		if (img.cols <= 0 or img.rows <= 0)
			return false;
		cimbar::mat_to_gl::load_gl_texture(_texid, img, _texdims);
		_texdims = {static_cast<unsigned>(img.cols), static_cast<unsigned>(img.rows)};
		return true;
	}

	void draw()
	{
		if (!good())
			return;

		glUseProgram(_p.id());

		glBindVertexArray(_vao);

		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _texid);
		glUniform1i(_p.uniform_tex(), 0);

		// pass in rotation matrix
		std::array<GLfloat, 4> rot = *_rotation;
		glUniformMatrix2fv(_p.uniform_rot(), 1, false, rot.data());

		// pass in transform vector
		std::pair<GLfloat, GLfloat> tform = *_shake;
		glUniform2f(_p.uniform_tform(), tform.first, tform.second);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Unbind
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);

		glUseProgram(0);
	}

	void rotate(unsigned i=1)
	{
		if (i == 0)
			_rotation.reset();
		else
			++_rotation;
	}

	void shake(unsigned i=1)
	{
		if (i == 0)
			_shake.reset();
		else
			++_shake;
	}

protected:
	void init()
	{
		glGenTextures(1, &_texid);
		glGenBuffers(1, &_vbo);
		glGenVertexArrays(1, &_vao);

		// Setup VBO
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), PLANE, GL_STATIC_DRAW);

		// Setup VAO
		GLint vertexPositionAttribute = glGetAttribLocation(_p.id(), "vert");
		glBindVertexArray(_vao);
		glEnableVertexAttribArray(vertexPositionAttribute);
		glVertexAttribPointer(vertexPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

protected:
	gl_2d_display_program _p;
	GLuint _texid;
	vec_xy _texdims;
	GLuint _vbo;
	GLuint _vao;

	std::array<std::pair<GLfloat, GLfloat>, 4> _shakePos;
	loop_iterator<decltype(_shakePos)> _shake;
	loop_iterator<decltype(ROTATIONS)> _rotation;
};

}
