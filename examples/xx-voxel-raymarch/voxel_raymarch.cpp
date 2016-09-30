/*
* Copyright 2016 Rasmus Christian Pedersen. All rights reserved.
* License: http://www.opensource.org/licenses/BSD-2-Clause
*/

#include "common.h"
#include "bgfx_utils.h"

#include "imgui/imgui.h"

#include <string>
#include <fstream>
#include <iostream>

struct Vertex
{
	  float m_x;
	  float m_y;
	  float m_z;

	  static void init()
	  {
		    ms_decl
			      .begin()
			      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
						.end();
	  };

	  static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl Vertex::ms_decl;


class Raymarching : public entry::AppI
{
    void init(int /*_argc*/, char** /*_argv*/) BX_OVERRIDE
    {
        m_width  = 1280;
        m_height = 720;
        m_debug  = BGFX_DEBUG_TEXT;
        m_reset  = BGFX_RESET_VSYNC;

        bgfx::init();
        bgfx::reset(m_width, m_height, m_reset);

        // Enable debug text.
        bgfx::setDebug(m_debug);

        // Set view 0 clear state.
        bgfx::setViewClear(0
                           , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
                           , 0x303030ff
                           , 1.0f
                           , 0
                           );

        // Create vertex stream declaration.
        Vertex::init();

        parseVolumeHeader("vismale.dat");
        m_memory = loadVolumeFromFile("vismale.raw");

        m_boxMin[0] = 0.5f;
        m_boxMin[1] = 0.5f;
        m_boxMin[2] = 0.5f;
        m_boxMin[3] = 0.5f;

        m_boxMax[0] = /* m_boxMin[0] + */ float(m_volumeSize[0]) * m_voxelSize[0];
        m_boxMax[1] = /* m_boxMin[1] + */ float(m_volumeSize[1]) * m_voxelSize[1];
        m_boxMax[2] = /* m_boxMin[2] + */ float(m_volumeSize[2]) * m_voxelSize[2];
        m_boxMax[3] = 0.0f;

        m_texSize[0] = float(m_volumeSize[0]);
        m_texSize[1] = float(m_volumeSize[1]);
        m_texSize[2] = float(m_volumeSize[2]);
        m_texSize[3] = 0.0f;

        static const Vertex vertices[8] =
        {
            { m_boxMin[0] + 0.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 1.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 1.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 1.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 1.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 1.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 0.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 0.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 1.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 1.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 0.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 1.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 0.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 1.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 0.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 1.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 1.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 0.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 0.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 0.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 0.0f * m_volumeSize[2] * m_voxelSize[2] },
            { m_boxMin[0] + 1.0f * m_volumeSize[0] * m_voxelSize[0], m_boxMin[1] + 0.0f * m_volumeSize[1] * m_voxelSize[1], m_boxMin[2] + 0.0f * m_volumeSize[2] * m_voxelSize[2] },
        };

        // Create static vertex buffer.
        m_vbh = bgfx::createVertexBuffer(
                                         // Static data can be passed with bgfx::makeRef
                                         bgfx::makeRef(vertices, sizeof(vertices))
                                         , Vertex::ms_decl
                                         );

        static const uint16_t indices[36] =
        {
            0, 1, 2, // 0
            1, 3, 2,
            4, 6, 5, // 2
            5, 6, 7,
            0, 2, 4, // 4
            4, 2, 6,
            1, 5, 3, // 6
            5, 7, 3,
            0, 4, 1, // 8
            4, 5, 1,
            2, 3, 6, // 10
            6, 3, 7,
        };

        // Create static index buffer.
        m_ibh = bgfx::createIndexBuffer(
                                        // Static data can be passed with bgfx::makeRef
                                        bgfx::makeRef(indices, sizeof(indices))
                                        );

        u_isovalue = bgfx::createUniform("u_isovalue", bgfx::UniformType::Vec4);
        u_eyePos = bgfx::createUniform("u_eye", bgfx::UniformType::Vec4);
        u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
        u_boxMin = bgfx::createUniform("u_boxMin", bgfx::UniformType::Vec4);
        u_boxMax = bgfx::createUniform("u_boxMax", bgfx::UniformType::Vec4);
        u_texSize = bgfx::createUniform("u_texSize", bgfx::UniformType::Vec4);
        u_texMatrix = bgfx::createUniform("u_texMatrix", bgfx::UniformType::Mat4);
        u_volume = bgfx::createUniform("u_volume", bgfx::UniformType::Int1);

        // Load volume from file.
        const uint32_t flags = BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP | BGFX_TEXTURE_W_CLAMP;
        m_texture = bgfx::createTexture3D(m_volumeSize[0],
                                          m_volumeSize[1],
                                          m_volumeSize[2],
                                          0,
                                          bgfx::TextureFormat::R8,
                                          flags,
                                          m_memory);

        // Create texture matrix.
        bx::mtxScale(m_texMatrix,
            1.0f / float(m_volumeSize[0] * m_voxelSize[0]),
            1.0f / float(m_volumeSize[1] * m_voxelSize[1]),
            1.0f / float(m_volumeSize[2] * m_voxelSize[2]));

        // Create program from shaders.
        m_program = loadProgram("vs_voxel_raymarch", "fs_voxel_raymarch");

        m_isovalue[0] = 0.225f;

        m_scrollArea = 0;

        // Imgui.
        imguiCreate();

        m_timeOffset = bx::getHPCounter();
    }

    virtual int shutdown() BX_OVERRIDE
    {
        // Cleanup.
        bgfx::destroyIndexBuffer(m_ibh);
        bgfx::destroyVertexBuffer(m_vbh);
        bgfx::destroyProgram(m_program);
        bgfx::destroyTexture(m_texture);
        bgfx::destroyUniform(u_isovalue);
        bgfx::destroyUniform(u_eyePos);
        bgfx::destroyUniform(u_lightPos);
        bgfx::destroyUniform(u_boxMin);
        bgfx::destroyUniform(u_boxMax);
        bgfx::destroyUniform(u_volume);
        bgfx::destroyUniform(u_texSize);
        bgfx::destroyUniform(u_texMatrix);

        // Shutdown bgfx.
        bgfx::shutdown();

        return 0;
    }

    bool update() BX_OVERRIDE
    {
        if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
        {
            imguiBeginFrame(m_mouseState.m_mx
                    ,  m_mouseState.m_my
                    , (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
                    | (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
                    | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
                    ,  m_mouseState.m_mz
                    , m_width
                    , m_height
                    );

            imguiBeginScrollArea("Settings", m_width - m_width / 5 - 10, 10, m_width / 5, m_height / 2, &m_scrollArea);
            imguiSeparatorLine();
            imguiSlider("isovalue", m_isovalue[0], 0.0f, 1.0f, 0.001f);
            imguiSeparator();

            imguiEndScrollArea();
            imguiEndFrame();

            int64_t now = bx::getHPCounter();
            static int64_t last = now;
            const int64_t frameTime = now - last;
            last = now;
            const double freq = double(bx::getHPFrequency() );
            const double toMs = 1000.0/freq;

            float time = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );

            // Use debug font to print information about this example.
            bgfx::dbgTextClear();
            bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/xx-voxel-raymarch");
            bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: empty.");
            bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

            const float eye[4] =
            {
                0.0f,
                0.0f,
                -1.5f * m_volumeSize[2] * m_voxelSize[2],
                1.0f
            };

            const float at[3] =
            {
                0.0f,
                0.0f,
                0.0f
            };

            const float lightPos[4] =
            {
                1.0f * m_volumeSize[0] * m_voxelSize[0],
                1.0f * m_volumeSize[1] * m_voxelSize[1],
                1.0f * m_volumeSize[2] * m_voxelSize[2],
                1.0f
            };

            // Set view and projection matrix for view 0.
            float view[16];
            bx::mtxLookAt(view, eye, at);

            float proj[16];
            bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 1000.0f, true);
            bgfx::setViewTransform(0, view, proj);

            // Set view 0 default viewport.
            bgfx::setViewRect(0, 0, 0, m_width, m_height);

            // This dummy draw call is here to make sure that view 0 is cleared
            // if no other draw calls are submitted to view 0.
            bgfx::touch(0);

            float translate[16];
            bx::mtxTranslate(translate,
                -0.5f * m_volumeSize[0] * m_voxelSize[0],
                -0.5f * m_volumeSize[1] * m_voxelSize[1],
                -0.5f * m_volumeSize[2] * m_voxelSize[2]);

            float rotate[16];
            bx::mtxRotateXY(rotate, time, time * 0.37f);
            //bx::mtxIdentity(rotate);

            float model[16];
            bx::mtxMul(model, translate, rotate);

            float invModel[16];
            bx::mtxInverse(invModel, model);

            float eyeModelSpace[4];
            bx::vec4MulMtx(eyeModelSpace, eye, invModel);

            float lightPosModelSpace[4];
            bx::vec4MulMtx(lightPosModelSpace, lightPos, invModel);

            bgfx::setUniform(u_isovalue, m_isovalue);
            bgfx::setUniform(u_eyePos, eyeModelSpace);
            bgfx::setUniform(u_lightPos, eyeModelSpace);
            bgfx::setUniform(u_boxMin, m_boxMin);
            bgfx::setUniform(u_boxMax, m_boxMax);
            bgfx::setUniform(u_texSize, m_texSize);
            bgfx::setUniform(u_texMatrix, m_texMatrix);

            // Set volume texture.
            bgfx::setTexture(0, u_volume, m_texture);

            // Set model matrix for rendering.
            bgfx::setTransform(model);

            // Set vertex and index buffer.
            bgfx::setVertexBuffer(m_vbh);
            bgfx::setIndexBuffer(m_ibh);

            // Set render states.
            bgfx::setState(BGFX_STATE_DEFAULT);

            // Submit primitive for rendering to view 0.
            bgfx::submit(0, m_program);

            // Advance to next frame. Rendering thread will be kicked to
            // process submitted rendering primitives.
            bgfx::frame();

            return true;
        }

        return false;
    }

    void parseVolumeHeader(const std::string& filename)
    {
        char filePath[512];
        strcpy(filePath, "volumes/");
        strcat(filePath, filename.c_str());

        std::ifstream ifs(filePath);
        ifs >> m_volumeSize[0] >> m_volumeSize[1] >> m_volumeSize[2];
        ifs >> m_voxelSize[0] >> m_voxelSize[1] >> m_voxelSize[2];
        ifs.close();
    }

    const bgfx::Memory* loadVolumeFromFile(const std::string& filename)
    {
        char filePath[512];

        const uint32_t bpv = 1;
        const uint32_t memorySize = m_volumeSize[0] * m_volumeSize[1] * m_volumeSize[2] * bpv;

        const bgfx::Memory* memory = bgfx::alloc(memorySize);

        strcpy(filePath, "volumes/");
        strcat(filePath, filename.c_str());

        std::ifstream ifs;
        ifs.open(filePath, std::ios::in | std::ios::binary);
        ifs.read(reinterpret_cast<char*>(memory->data), memory->size);
        ifs.close();

        return memory;
    }

    bgfx::ProgramHandle m_program;
    bgfx::TextureHandle m_texture;
    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle m_ibh;

    bgfx::UniformHandle u_isovalue;
    bgfx::UniformHandle u_eyePos;
    bgfx::UniformHandle u_lightPos;
    bgfx::UniformHandle u_boxMin;
    bgfx::UniformHandle u_boxMax;
    bgfx::UniformHandle u_texSize;
    bgfx::UniformHandle u_texMatrix;
    bgfx::UniformHandle u_volume;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_debug;
    uint32_t m_reset;
    int64_t m_timeOffset;

    float m_boxMin[4];
    float m_boxMax[4];
    float m_texSize[4];
    float m_texMatrix[16];

    float m_voxelSize[3];
    uint32_t m_volumeSize[3];

    const bgfx::Memory* m_memory;

    entry::MouseState m_mouseState;
    float m_isovalue[4];

    int32_t m_scrollArea;

};

ENTRY_IMPLEMENT_MAIN(Raymarching);
