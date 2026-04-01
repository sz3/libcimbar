/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "gl_2d_display.h"
#include "mat_to_gl.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <string>
#include <thread>

namespace cimbar {

class window_glfw
{
protected:
	struct runtime_ctx
	{
		unsigned width;
		unsigned height;
		unsigned padding;
	};

public:
	window_glfw(unsigned width, unsigned height, std::string title)
		: _ctx({width, height, 0})
	{
		if (!glfwInit())
		{
			_good = false;
			return;
		}

		_w = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		if (!_w)
		{
			_good = false;
			return;
		}
		glfwMakeContextCurrent(_w);
		glfwSwapInterval(1);
		glfwSetWindowUserPointer(_w, &_ctx);

		_display = std::make_shared<cimbar::gl_2d_display>(std::max(width, height));
		glGenTextures(1, &_texid);
		init_opengl(width, height);
	}

	~window_glfw()
	{
		if (_w)
		{
			glfwDestroyWindow(_w);
			_w = nullptr;
		}
		if (_texid)
			glDeleteTextures(1, &_texid);
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

	void auto_scale_to_window(unsigned padding=0)
	{
		if (!is_good())
			return;
		_ctx.padding = padding;
		glfwSetWindowAspectRatio(_w, _ctx.width, _ctx.height);
		auto fun = [](GLFWwindow* win, int w, int h) {
			if (h == 0)
				return;

			auto* appCtx = static_cast<runtime_ctx*>(glfwGetWindowUserPointer(win));
			float targetAspect = static_cast<float>(appCtx->width - appCtx->padding)/(appCtx->height - appCtx->padding);
			float currentAspect = static_cast<float>(w-appCtx->padding) / static_cast<float>(h-appCtx->padding);

			int viewportWidth, viewportHeight;
			int xOffset = 0, yOffset = 0;

			if (currentAspect > targetAspect)
			{
				viewportHeight = h - appCtx->padding;
				viewportWidth = static_cast<int>(h * targetAspect) - appCtx->padding;
				xOffset = (w - viewportWidth) / 2;
				yOffset = appCtx->padding / 2;
			}
			else
			{
				viewportWidth = w - appCtx->padding;
				viewportHeight = static_cast<int>(w / targetAspect) - appCtx->padding;
				yOffset = (h - viewportHeight) / 2;
				xOffset = appCtx->padding / 2;
			}
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(xOffset, yOffset, viewportWidth, viewportHeight);
		};
		glfwSetFramebufferSizeCallback(_w, fun);
	}

	void resize(unsigned width, unsigned height)
	{
		if (_w)
		{
			_ctx.width = width;
			_ctx.height = height;
			glfwSetWindowAspectRatio(_w, width, height);
			glfwSetWindowSize(_w, width +_ctx.padding, height+_ctx.padding);
			init_opengl(width +_ctx.padding, height+_ctx.padding);
		}
	}

	void rotate(unsigned i=1)
	{
		if (_display)
			_display->rotate(i);
	}

	void shake(unsigned i=1)
	{
		if (_display)
			_display->shake(i);
	}

	void clear()
	{
		if (_display)
		{
			_display->clear();
			swap();
		}
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		std::chrono::time_point start = std::chrono::high_resolution_clock::now();

		if (_display)
		{
			cimbar::mat_to_gl::load_gl_texture(_texid, img);
			_display->draw(_texid);

			swap();
			poll();
		}

		unsigned millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		if (delay > millis)
			std::this_thread::sleep_for(std::chrono::milliseconds(delay-millis));
	}

	unsigned width() const
	{
		return _ctx.width;
	}

	unsigned height() const
	{
		return _ctx.height;
	}

protected:
	void init_opengl(int width, int height)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
	}

	void swap()
	{
		// show next frame
		glfwSwapBuffers(_w);
	}

	void poll()
	{
		glfwPollEvents();
	}

protected:
	GLFWwindow* _w = nullptr;
	runtime_ctx _ctx;
	GLuint _texid;
	std::shared_ptr<cimbar::gl_2d_display> _display;
	bool _good = true;
};

}
