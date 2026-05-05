/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <string>

namespace cimbar {

class gl_program
{
public:
	template<typename... StringList>
	gl_program(GLuint vertexShader, GLuint fragmentShader, StringList... vertextVarName)
	{
		_p = build(vertexShader, fragmentShader, vertextVarName...);
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
	template<typename... StringList>
	GLuint build(GLuint vertexShader, GLuint fragmentShader, StringList... vertextVarName)
	{
		GLuint prog = glCreateProgram();
		glAttachShader(prog, vertexShader);
		glAttachShader(prog, fragmentShader);

		int varI = 0;
		(
			[prog, &varI](const std::string& varname) {
				glBindAttribLocation(prog, varI++, varname.c_str());
			}
			(vertextVarName), ...
		);
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
