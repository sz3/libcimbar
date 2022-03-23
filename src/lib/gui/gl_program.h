/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <iostream>
#include <string>

namespace cimbar {

class gl_program
{
public:
	gl_program(GLuint vertexShader, GLuint fragmentShader, const std::string& vertextVarName)
	{
		_p = build(vertexShader, fragmentShader, vertextVarName);
	}

	~gl_program()
	{
		if (_p)
			glDeleteProgram(_p);
	}

	operator GLuint() const
	{
		return _p;
	}

protected:
	GLuint build(GLuint vertexShader, GLuint fragmentShader, const std::string& vertextVarName)
	{
		GLuint prog = glCreateProgram();
		glAttachShader(prog, vertexShader);
		glAttachShader(prog, fragmentShader);
		glBindAttribLocation(prog, 0, vertextVarName.c_str());
		glLinkProgram(prog);

		GLint res;
		glGetProgramiv(prog, GL_LINK_STATUS, &res);
		if (!res)
		{
			std::cerr << "Error linking program" << std::endl;
			glDeleteProgram(prog);
			return 0;
		}

		return prog;
	}

protected:
	GLuint _p;
};

}
