#pragma once

#include <core/prerequisites.hpp>
#include <core/win/window.hpp>
#include <ext/glew/glew.h>
#include <core/ogl/shaders.hpp>
#include <core/ogl/gltex2d.hpp>
#include <core/math/vector3.hpp>
#include <core/utils/color.hpp>

#pragma comment(lib, "opengl32.lib")

#ifdef _DEBUG
#pragma comment(lib, "glew32sd.lib")
#else
#pragma comment(lib, "glew32s.lib")
#endif

namespace core {
	class glrenderer {
		window mWnd;
		HDC mDC;
		HGLRC mCtx;
		mat4f mProjection;
		mat4f mView;
		mat4f mViewProjection;
		std::unique_ptr<glprogram> mDefaultProgram;
		std::unique_ptr<glprogram> mTexProgram;
		std::unique_ptr<vertexarray> mDefaultVAO;

		void initpixelformat();
		void initshaders();
		void projection(const mat4f& m);
	public:
		glrenderer(const window& target);
		~glrenderer();

		void view(const mat4f& m);

		void lookat(vector3f from, vector3f to=vector3f(0.f,0.f,0.f), vector3f up=vector3f(0.f,1.f,0.f));
		void perspective();
		void ortho();
		void projSetScale(vector3f scale);

		void triangles(const core::color& color, const vector3f* triangles, int ntriangles);
		void line(const core::color& color, float x1, float y1, float x2, float y2);
		void line(const core::color& color, const vector3f& from, const vector3f& to);
		void lines(const core::color& color, vector3f* lines, int linecount);
		void rect(const core::color& color, float x1, float y1, float x2, float y2);
		void texrect(const gltex2d& tex, float x1, float y1, float x2, float y2, const core::color& color=Color::White);

		void clear(const core::color& color);
		void flush();
	};
}
