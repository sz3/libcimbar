#pragma once

#include "mat_to_gl.h"

#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <string>

class window_glfw
{
public:
	window_glfw(unsigned width, unsigned height, std::string title)
	{
		if (!glfwInit())
		{
			_good = false;
			return;
		}

		_w = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		if (!_w)
			_good = false;
		else
			glfwMakeContextCurrent(_w);
	}

	~window_glfw()
	{
		if (_w)
			glfwDestroyWindow(_w);
		glfwTerminate();
	}

	bool is_good() const
	{
		return _good;
	}

	bool should_close() const
	{
		return glfwWindowShouldClose(_w);
	}

	void swap()
	{
		/* Swap front and back buffers */
		glfwSwapBuffers(_w);
	}

	void poll()
	{
		glfwPollEvents();
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		cimbar::mat_to_gl::draw(img);
		swap();
		poll();
	}

protected:
	GLFWwindow* _w;
	bool _good = true;
};
