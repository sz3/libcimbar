#pragma once

#include <GL/glew.h>
#include <opencv2/opencv.hpp>

namespace cimbar {
namespace mat_to_gl {

	GLuint create_gl_texture(const cv::Mat& mat)
	{
		GLuint texid;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);

		// not sure whether we need this or not
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLenum format = GL_BGR;
		switch (mat.channels())
		{
			case 1:
				format = GL_LUMINANCE;
				break;
			case 4:
				format = GL_BGRA;
				break;
			default:
				;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.data);
		return texid;
	}

	void draw(const cv::Mat& mat)
	{
		// clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_TEXTURE_2D);
		GLuint tex = create_gl_texture(mat);

		// might have to mirror direction here??
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex2i(0, 0);
		glTexCoord2i(0, 1);
		glVertex2i(0, mat.rows);
		glTexCoord2i(1, 1);
		glVertex2i(mat.cols, mat.rows);
		glTexCoord2i(1, 0);
		glVertex2i(mat.cols, 0);
		glEnd();

		glDeleteTextures(1, &tex);
		glDisable(GL_TEXTURE_2D);
	}

}
}
