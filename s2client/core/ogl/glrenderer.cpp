#include "glrenderer.hpp"

#include <core/io/logger.hpp>

#pragma comment(lib, "glu32.lib")

namespace core {
	void glrenderer::initpixelformat() {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd  
            1,                     // version number  
            PFD_DRAW_TO_WINDOW |   // support window  
            PFD_SUPPORT_OPENGL |   // support OpenGL  
            PFD_DOUBLEBUFFER,      // double buffered  
            PFD_TYPE_RGBA,         // RGBA type  
            24,                    // 24-bit color depth  
            0, 0, 0, 0, 0, 0,      // color bits ignored  
            0,                     // no alpha buffer  
            0,                     // shift bit ignored  
            0,                     // no accumulation buffer  
            0, 0, 0, 0,            // accum bits ignored  
            32,                    // 32-bit z-buffer  
            0,                     // no stencil buffer  
            0,                     // no auxiliary buffer  
            PFD_MAIN_PLANE,        // main layer  
            0,                     // reserved  
            0, 0, 0                // layer masks ignored  
        };
        int  iPixelFormat;

        // get the best available match of pixel format for the device context   
        iPixelFormat = ChoosePixelFormat(mDC, &pfd);

        // make that the pixel format of the device context  
        SetPixelFormat(mDC, iPixelFormat, &pfd);
	}
    void glrenderer::initshaders() {
        mDefaultProgram = std::make_unique<glprogram>();
        mDefaultProgram->attachVertexShader(
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform vec4 uFlatColor;\n"
            "uniform mat4 matProject;\n"
            "out vec4 fColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = matProject * vec4(aPos, 1.0);\n"
            "   fColor = uFlatColor;\n"
            "}\0"
        );
        mDefaultProgram->attachFragmentShader(
            "#version 330 core\n"
            "in vec4 fColor;\n"
            "out vec4 oFragColor;\n"
            "void main()\n"
            "{\n"
            "   oFragColor = fColor;\n"
            "}\0"
        );
        mDefaultProgram->link();
        mTexProgram = std::make_unique<glprogram>();
        mTexProgram->attachVertexShader(
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 2) in vec2 aTexCoord;\n"
            "uniform vec4 uFlatColor;\n"
            "uniform mat4 matProject;\n"
            "out vec4 fColor;\n"
            "out vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = matProject * vec4(aPos, 1.0);\n"
            "   fColor = uFlatColor;\n"
            "   TexCoord = aTexCoord;\n"
            "}\0"
        );
        mTexProgram->attachFragmentShader(
            "#version 330 core\n"
            "in vec4 fColor;\n"
            "in vec2 TexCoord;\n"
            "uniform sampler2D tex;\n"
            "out vec4 oFragColor;\n"
            "void main()\n"
            "{\n"
            "   oFragColor = texture(tex, TexCoord) * fColor;\n"
            "}\0"
        );
        mTexProgram->link();
        auto orthoProj = mat4f::ortho(0, (float)mWnd.width(), (float)mWnd.height(), 0, 0.f, 1.f);
        projection(orthoProj);
    }
    void glrenderer::projection(const mat4f& m) {
        mProjection = m;
        mViewProjection = mView * mProjection;
        mTexProgram->use(); mTexProgram->set("matProject", mViewProjection);
        mDefaultProgram->use(); mDefaultProgram->set("matProject", mViewProjection);

    }
    void glrenderer::view(const mat4f& m) {
        mView = m;
        mViewProjection = mView * mProjection;
        mTexProgram->use(); mTexProgram->set("matProject", mViewProjection);
        mDefaultProgram->use(); mDefaultProgram->set("matProject", mViewProjection);
    }

    static void APIENTRY ogl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        core::error("source %Xh type %Xh id %Xh severity %Xh length %Xh message %s\n", source, type, id, severity, length, message);
    }

    glrenderer::glrenderer(const window& target) : mWnd(target) {
        mDC = GetDC(mWnd.handle());
        initpixelformat();
        mCtx = wglCreateContext(mDC);
        wglMakeCurrent(mDC, mCtx);
        auto err = glewInit();
        assert(err == GLEW_OK);

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(ogl_debug_proc, nullptr);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        initshaders();
        mDefaultVAO.reset(new vertexarray());
        //mDefaultVAO = std::make_shared<vertexarray>();
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), {
            { -1.0f, -1.0f, 0.0f },
            {  1.0f, -1.0f, 0.0f },
            {  1.0f,  1.0f, 0.0f },
            { -1.0f,  1.0f, 0.0f },
        });
		core::info("glrenderer initialized with gl v%s\n", glGetString(GL_VERSION));
        glViewport(0, 0, mWnd.width(), mWnd.height());
        
        mView = mat4f::identity();
        mProjection = mat4f::identity();

        //clear(0.5f, 0.1f, 0.05f, 1.0f);
        flush();
    }

	glrenderer::~glrenderer() {
		wglDeleteContext(mCtx);
		ReleaseDC(mWnd.handle(), mDC);
	}

    void glrenderer::lookat(vector3f from, vector3f to, vector3f up) {
        auto f = (to - from).unit();
        auto s = f.cross(up);
        auto u = s.cross(f);
        mat4f lookat{
            s.x, s.y, s.z, 0.f,
            u.x, u.y, u.z, 0.f,
            -f.x, -f.y, -f.z, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
        auto tr = mat4f::translate(-from.x, -from.y, -from.z);
        auto proj = mProjection * lookat * tr;
        projection(proj);
    }

    void glrenderer::perspective() {
        auto proj = mat4f::perspective(70.0f, (float)mWnd.width() / mWnd.height(), 0.001f, 1000000.0f);
        projection(proj);
    }

    void glrenderer::ortho() {
        auto proj = mat4f::ortho(0.f, (float)mWnd.width(), (float)mWnd.height(), 0.f, -1.f, 1.f);
        projection(proj);
    }

    void glrenderer::projSetScale(vector3f scale) {
        mProjection._11 = scale.x;
        mProjection._22 = scale.y;
        mProjection._33 = scale.z;
        projection(mProjection);
    }

    void glrenderer::triangles(const core::color& color, const vector3f* triangles, int ntriangles) {
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), triangles, ntriangles * 3);
        mDefaultVAO->bind();
        mDefaultProgram->use();
        mDefaultProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_TRIANGLES, 0, ntriangles * 3);
    }

    void glrenderer::line(const core::color& color, float x1, float y1, float x2, float y2) {
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), {
            { x1, y1, 0.0f },
            { x2, y2, 0.0f }
        });
        mDefaultVAO->bind();
        mDefaultProgram->use();
        mDefaultProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_LINES, 0, 2);
    }

    void glrenderer::line(const core::color& color, const vector3f& from, const vector3f& to) {
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), {
            { from.x, from.y, from.z },
            { to.x, to.y, to.z }
        });
        mDefaultVAO->bind();
        mDefaultProgram->use();
        mDefaultProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_LINES, 0, 2);
    }

    void glrenderer::lines(const core::color& color, vector3f* lines, int linecount) {
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), lines, linecount*2);
        mDefaultVAO->bind();
        mDefaultProgram->use();
        mDefaultProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_LINES, 0, linecount * 2);
    }

    void glrenderer::rect(const core::color& color, float x1, float y1, float x2, float y2) {
        mDefaultVAO->vertexdata(mDefaultProgram->attribloc("aPos"), {
            { x1, y1, 0.0f },
            { x2, y1, 0.0f },
            { x1, y2, 0.0f },
            { x2, y2, 0.0f }
        });
        mDefaultVAO->bind();
        mDefaultProgram->use();
        mDefaultProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void glrenderer::texrect(const gltex2d& tex, float x1, float y1, float x2, float y2, const core::color& color) {
        tex.bind();
        mDefaultVAO->vertexdata(mTexProgram->attribloc("aPos"), {
            { x1, y1, 0.0f },
            { x2, y1, 0.0f },
            { x1, y2, 0.0f },
            { x2, y2, 0.0f }
        });
        mDefaultVAO->vertexdata(mTexProgram->attribloc("aTexCoord"), {
            { 0.0f, 1.0f },
            { 1.0f, 1.0f },
            { 0.0f, 0.0f },
            { 1.0f, 0.0f }
        });
        mDefaultVAO->bind();
        mTexProgram->use();
        mTexProgram->set<core::color>("uFlatColor", color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void glrenderer::clear(const core::color& color) {
        glClearColor(color.R, color.G, color.B, color.A);
        glClearDepth(0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void glrenderer::flush() {
        glFlush();
        wglSwapLayerBuffers(mDC, WGL_SWAP_MAIN_PLANE);
        UpdateWindow(mWnd.handle());
    }
}
