/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <iostream>

namespace cimbar {

class gl_shader
{
public:
	gl_shader(GLenum type, const std::string& source)
	{
		_s = compile(type, source.c_str());
	}

	operator GLuint() const
	{
		return _s;
	}

	bool good() const
	{
		return _s != 0;
	}

public:
	static GLuint compile(GLenum type, const char* source)
	{
		GLuint shader = glCreateShader(type);
		if (shader == 0)
		{
			 std::cerr << "Failed to create shader";
			return 0;
		}

		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);

		GLint res;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
		if (!res)
		{
			 std::cerr << "Error compiling shader" << std::endl;
			GLchar info[1000];
			glGetShaderInfoLog(shader, 1000, NULL, info);
			 std::cerr << info << std::endl;
			glDeleteShader(shader);
			return 0;
		}

		return shader;
	}

protected:
	GLuint _s;
};

}
