//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//
#include <iostream>
#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "assimp_model_loading.h"
#include "buffer_management.h"

#define BINDING(b) b

bool mode;

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;

    GLuint vaoHandle = 0;

    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i) {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j) {
                if (program.vertexInputLayout.attributes[i].location != submesh.vertexBufferLayout.attributes[j].location)
                    continue;
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
            assert(attributeWasLinked);
        }

        glBindVertexArray(0);
    }

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void Init(App* app)
{

    app->camera.cameraPos = { -0.368, 6.492, 8.699 };

    InitGPUInfo(app);

    InitModes(app);

    InitBuffers(app);

    CreateEntities(app);

}

void Gui(App* app)
{

    ImGui::StyleColorsClassic();
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    
    // GPU info
    ImGui::Separator();
    ImGui::Text("Vendor: %s", app->vendor);
    ImGui::Text("Renderer: %s", app->renderer);
    ImGui::Text("Version: %s", app->version);
    ImGui::Text("GLSL Version: %s", app->shadingLanguageVersion);
    ImGui::Text("Extensions: %s", app->extensions);

    //Camera info
    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::InputFloat3("Transform", &app->camera.cameraPos.x, "%.3f");
    ImGui::InputFloat3("Rotation", &app->camera.cameraUp.x, "%.3f");

    ImGui::Separator();

    // Lights info
    static const char* controllers[] = {"Albedo", "Normals", "Depth", "Position"};

	ImGui::Text("Lights");

	ImGui::Checkbox("Show Light Gizmo", &app->showGizmo);

	if (ImGui::CollapsingHeader("Light Inspector")) {

        // Lights counter
        int dirCount = 0;
        int pointCount = 0;

		for (int i = 0; i < app->lights.size(); ++i) {
			ImGui::PushID(i);
            
			if (app->lights[i].type == 0) { //Directional
                dirCount++;
                ImGui::Text("Directrional Light");
                ImGui::SameLine();
                ImGui::Text("%i", dirCount);
				ImGui::DragFloat3("direction", glm::value_ptr(app->lights[i].direction), 0.01f);
			}
			else {
                pointCount++;
                ImGui::Text("Point Light");
                ImGui::SameLine();
                ImGui::Text("%i", pointCount);
				ImGui::DragFloat3("transform", glm::value_ptr(app->lights[i].position), 0.01f);
			}
			ImGui::DragFloat3("color", glm::value_ptr(app->lights[i].color), 0.01f);
			ImGui::DragFloat("intensity", &app->lights[i].intensity, 0.01f);
			ImGui::PopID();
			ImGui::NewLine();
		}
	}

    ImGui::Separator();
    const char* controller[] = { "Deferred", "Forward" };
    ImGui::Text("Rendering");
    static int select = 0;
    if (ImGui::BeginCombo("Type", controller[select]))
    {
        for (int i = 0; i < 2; ++i)
        {
            if (ImGui::Selectable(controller[i]))
            {
                select = i;
            }
        }
        ImGui::EndCombo();
    }

    switch (select)
    {
    case 0:
        app->mode = Mode::DEFERRED;
        break;
    case 1:
        app->mode = Mode::FORWARD;
        break;
    default:
        break;
    }

	ImGui::Separator();

    ImGui::Checkbox("Show relief", &app->showRelief);

    ImGui::Separator();

    ImGui::Checkbox("Skybox", &app->showCubeMap);

    ImGui::Separator();

    // Render info
    static int sel = 0;
    ImGui::Text("Target render");
    if (ImGui::BeginCombo("Target", controllers[sel])) {
        for (int i = 0; i < 4; ++i)
            if (ImGui::Selectable(controllers[i])) sel = i;
        ImGui::EndCombo();
    }

    ImGui::Text("Chosen texture");
    GLuint texture = 0;
    switch (sel)
    {
    case 0:
        texture = app->albedoController;
        break;
    case 1:
        texture = app->normalsController;
        break;
    case 2:
        texture = app->depthController;
        break;
    case 3:
        texture = app->positionController;
        break;
    default:
        break;
    }

	

    ImGui::Image((ImTextureID)texture, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
    ImGui::Separator();
    ImGui::Text("Albedo");
    ImGui::Image((ImTextureID)app->albedoController, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
    ImGui::Text("Normals");
    ImGui::Image((ImTextureID)app->normalsController, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
    ImGui::Text("Depth");
    ImGui::Image((ImTextureID)app->depthController, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
    ImGui::Text("Position");
    ImGui::Image((ImTextureID)app->positionController, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));

    ImGui::End();
}

void Update(App* app)
{

	const float cameraSpeed = 2.5f * app->deltaTime; // adjust accordingly
	if (app->input.keys[K_W] == ButtonState::BUTTON_PRESSED)
		app->camera.cameraPos += cameraSpeed * app->camera.cameraFront;
	if (app->input.keys[K_S] == ButtonState::BUTTON_PRESSED)
		app->camera.cameraPos -= cameraSpeed * app->camera.cameraFront;
	if (app->input.keys[K_A] == ButtonState::BUTTON_PRESSED)
		app->camera.cameraPos -= glm::normalize(glm::cross(app->camera.cameraFront, app->camera.cameraUp)) * cameraSpeed;
	if (app->input.keys[K_D] == ButtonState::BUTTON_PRESSED)
		app->camera.cameraPos += glm::normalize(glm::cross(app->camera.cameraFront, app->camera.cameraUp)) * cameraSpeed;
	if (app->input.keys[K_R] == ButtonState::BUTTON_PRESSED) 
		app->camera.cameraPos += app->camera.cameraUp * 20.f * app->deltaTime;
	if (app->input.keys[K_F] == ButtonState::BUTTON_PRESSED) 
		app->camera.cameraPos -= app->camera.cameraUp * 20.f * app->deltaTime;


	if (app->input.mouseButtons[LEFT] == ButtonState::BUTTON_PRESSED)
	{
		app->camera.rotating = true;

		app->camera.yaw += app->input.mouseDelta.x * app->deltaTime * 20.f;
		app->camera.pitch -= app->input.mouseDelta.y * app->deltaTime * 20.f;

		if (app->camera.pitch > 89.0f)
			app->camera.pitch = 89.0f;
		if (app->camera.pitch < -89.0f)
			app->camera.pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		direction.y = sin(glm::radians(app->camera.pitch));
		direction.z = sin(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		app->camera.cameraFront = glm::normalize(direction);
	}
	else
		app->camera.rotating = false;

}

void Render(App* app)
{
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferController);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

   
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    switch (app->mode)
    {
        case TEXTUREDQUAD:
            {
            
            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            
            const Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);

            
            glBindVertexArray(app->vao);

            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
            }
            break;
        
        case DEFERRED:
        {
            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            MapBuffer(app->cBuffer, GL_WRITE_ONLY);
            app->globalParamsOffset = app->cBuffer.head;

            PushVec3(app->cBuffer, app->camera.cameraPos);
            PushUInt(app->cBuffer, app->lights.size());
            app->globalParamsSize = app->cBuffer.head - app->globalParamsOffset;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[app->toyDiffuseTexIdx].handle);
            glUniform1i(app->texturedMeshProgramIdx_Deferred, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->textures[app->toyNormalTexIdx].handle);
            glUniform1i(app->texturedMeshProgramIdx_RelieveNormal, 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->textures[app->toyHeightTexIdx].handle);
            glUniform1i(app->texturedMeshProgramIdx_RelieveHeight, 2);

            if(app->showCubeMap)
            RenderCubeMap(app);

            glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uShowRelief"), app->showRelief);
            for (int i = 0; i < app->entities.size(); ++i)
            {
                Model& model = app->models[app->entities[i].modelId];
                Mesh& mesh = app->meshes[model.meshIdx];

                AlignHead(app->cBuffer, app->uniformBlockAlignmentOffset);
                app->entities[i].localParamsOffset = app->cBuffer.head;
                PushMat4(app->cBuffer, app->entities[i].matrix);
                PushMat4(app->cBuffer, app->camera.GetViewMatrix(app->displaySize));
                app->entities[i].localParamsSize = app->cBuffer.head - app->entities[i].localParamsOffset;

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globalParamsOffset, app->globalParamsSize);
                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cBuffer.handle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshmaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.albedoTextureIdx].handle);
                    glUniform1i(app->programUniformTexture, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, NULL);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(app->programs[app->lightsProgramIdx].handle);

            glUniform1i(app->texturedMeshProgramIdx_uPosition, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->positionController);

            glUniform1i(app->texturedMeshProgramIdx_uNormals, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->normalsController);

            glUniform1i(app->texturedMeshProgramIdx_uAlbedo, 2);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->albedoController);

            //Cube


           


            AlignHead(app->cBuffer, app->uniformBlockAlignmentOffset);

            app->globalParamsOffset = app->cBuffer.head;

            PushVec3(app->cBuffer, app->camera.cameraPos);
            PushUInt(app->cBuffer, app->lights.size());

            for (int i = 0; i < app->lights.size(); ++i)
            {
                AlignHead(app->cBuffer, sizeof(glm::vec4));
                PushUInt(app->cBuffer, app->lights[i].type);
                PushVec3(app->cBuffer, app->lights[i].color);
                PushVec3(app->cBuffer, app->lights[i].direction);
                PushVec3(app->cBuffer, app->lights[i].position);
                PushFloat(app->cBuffer, app->lights[i].intensity);
            }
            app->globalParamsSize = app->cBuffer.head - app->globalParamsOffset;
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globalParamsOffset, app->globalParamsSize);
            UnmapBuffer(app->cBuffer);
            renderQuad();


			glBindFramebuffer(GL_READ_FRAMEBUFFER, app->frameBufferController);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			if (app->showGizmo) {

				glUseProgram(app->programs[app->drawLightsProgramIdx].handle);

				glUniformMatrix4fv(app->drawLightsProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(app->camera.GetViewMatrix(app->displaySize)));
				for (unsigned int i = 0; i < app->lights.size(); ++i) {

					glm::mat4 mat = glm::mat4(1.f);
					mat = glm::translate(mat, app->lights[i].position);
					glUniformMatrix4fv(app->drawLightsProgramIdx_uModel, 1, GL_FALSE, glm::value_ptr(mat));
					glUniform3fv(app->drawLightsProgramIdx_uLightColor, 1, glm::value_ptr(app->lights[i].color));
					if (app->lights[i].type == 0)
						RenderCube();
					else
					{
						RenderSphere();
					}

				}
			}

            break;
        }

        case FORWARD:
        {
            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            MapBuffer(app->cBuffer, GL_WRITE_ONLY);
            app->globalParamsOffset = app->cBuffer.head;

            PushVec3(app->cBuffer, app->camera.cameraPos);
            PushUInt(app->cBuffer, app->lights.size());

            for (auto& light : app->lights)
            {
                AlignHead(app->cBuffer, sizeof(glm::vec4));
                PushUInt(app->cBuffer, light.type);
                PushVec3(app->cBuffer, light.color);
                PushVec3(app->cBuffer, light.direction);

                app->globalParamsSize = app->cBuffer.head - app->globalParamsOffset;
            }

            for (int i = 0; i < app->entities.size(); ++i)
            {
                Model& model = app->models[app->entities[i].modelId];
                Mesh& mesh = app->meshes[model.meshIdx];

                AlignHead(app->cBuffer, app->uniformBlockAlignmentOffset);
                app->entities[i].localParamsOffset = app->cBuffer.head;

                float angle = 70;
                PushMat4(app->cBuffer, glm::rotate(glm::scale(app->entities[i].matrix, glm::vec3(2, 2, 2)), glm::radians(angle), glm::vec3(1, 0, 0)));
                PushMat4(app->cBuffer, app->camera.GetViewMatrix(app->displaySize));
                app->entities[i].localParamsSize = app->cBuffer.head - app->entities[i].localParamsOffset;
                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globalParamsOffset, app->globalParamsSize);
                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cBuffer.handle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);
                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(app->texturedMeshProgramIdx, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
                }
            }
            UnmapBuffer(app->cBuffer);
            glBindBuffer(GL_READ_FRAMEBUFFER, app->frameBufferController);
            glBindBuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            break;
        }

        default:;
    }


}

void CreateEntities(App* app)
{
    app->model = LoadModel(app, "Cube/Plane.obj");
    app->entities.push_back(Entity(glm::mat4(1.f), app->model));
    /*app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(5.f, 0.f, -4.f)), app->model));
	app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(-3.f, 0.f, -6.f)), app->model));*/


    app->lights.push_back(Light(LightType::DIRECTIONAL, vec3(0.8, 0.8, 0.8), vec3(0.0, -1.0, 1.0), vec3(4.f, 4.f, 0.f), 0.1)); 
    /*app->lights.push_back(Light(LightType::DIRECTIONAL, vec3(-0.5, -0.07, 0.23), vec3(-1.0, -1.0, 0.0), vec3(-4.f, -4.f, 0.f), 0.2));*/
    app->lights.push_back(Light(LightType::POINTT, vec3(0.0, 0.8, 0.9), vec3(0.4, -1.0, 1.0), vec3(2.f, -1.6f, 2.f), 0.7)); 
    /*app->lights.push_back(Light(LightType::POINTT, vec3(1.0, 0.9, 0.1), vec3(0.3, -1.0, 1.0), vec3(-2.f, 1.f, 2.f), 0.8)); 
    app->lights.push_back(Light(LightType::POINTT, vec3(1.0, 0.52, -0.15), vec3(0.6, -1.0, 1.0), vec3(6.4f, -0.05f, -2.5f), 0.7));
    app->lights.push_back(Light(LightType::POINTT, vec3(1.0, 0.04, 1.0), vec3(0.2, -1.0, 1.0), vec3(-4.9f, 0.86f, -5.6f), 0.8));    
    app->lights.push_back(Light(LightType::POINTT, vec3(1.0, -0.5, 0.0), vec3(0.0, -1.0, 1.0), vec3(4.0f, 1.76f, -6.53f), 2.0));
    app->lights.push_back(Light(LightType::POINTT, vec3(0.2, 0.8, 0.2), vec3(0.0, -1.0, 1.0), vec3(0.55f, 0.01f, -3.f), 0.9));*/

}

void InitGPUInfo(App* app)
{
    app->version = glGetString(GL_VERSION);
    app->renderer = glGetString(GL_RENDERER);
    app->vendor = glGetString(GL_VENDOR);
    app->shadingLanguageVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (int i = 0; i < num_extensions; ++i)
    {
        app->extensions = glGetStringi(GL_EXTENSIONS, GLuint(i));
    }
}

void InitModes(App* app)
{
    GLint maxBufferSize;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignmentOffset);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBufferSize);
    app->cBuffer = CreateBuffer(maxBufferSize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
    app->toyNormalTexIdx = LoadTexture2D(app, "Cube/toy_box_normal.png");
    app->toyHeightTexIdx = LoadTexture2D(app, "Cube/toy_box_disp.png");
    app->toyDiffuseTexIdx = LoadTexture2D(app, "Cube/toy_box_diffuse.png");

    app->mode = Mode::DEFERRED;

    switch (app->mode)
    {
    
    case Mode::DEFERRED:
    {
        app->texturedMeshProgramForward = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
        Program& texturedMeshProgram2 = app->programs[app->texturedMeshProgramIdx];
        app->texturedMeshProgramForward = glGetUniformLocation(texturedMeshProgram2.handle, "uTexture");
        texturedMeshProgram2.vertexInputLayout.attributes.push_back({ 0,3 });
        texturedMeshProgram2.vertexInputLayout.attributes.push_back({ 1,3 });
        texturedMeshProgram2.vertexInputLayout.attributes.push_back({ 2,2 });

        app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_GEOMETRY");
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        app->texturedMeshProgramIdx_Deferred = glGetUniformLocation(texturedMeshProgram.handle, "uAlbedoTexture");
        app->texturedMeshProgramIdx_RelieveNormal = glGetUniformLocation(texturedMeshProgram.handle, "uNormalTexture");
        app->texturedMeshProgramIdx_RelieveHeight = glGetUniformLocation(texturedMeshProgram.handle, "uBumpTexture");
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0,3 });
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1,3 });
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2,2 });
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 3,3 });
        texturedMeshProgram.vertexInputLayout.attributes.push_back({ 4,3 });

        app->cubeProgramIdx = LoadProgram(app,"shaders.glsl", "SHOW_CUBE");
        Program& cubeProgram = app->programs[app->cubeProgramIdx];
        cubeProgram.vertexInputLayout.attributes.push_back({ 0,3 });
        cubeProgram.vertexInputLayout.attributes.push_back({ 1,3 });

        app->skyBoxProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_SKY");
        Program& skyProgram = app->programs[app->skyBoxProgramIdx];
        skyProgram.vertexInputLayout.attributes.push_back({ 0,3 });

        app->lightsProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_LIGHT");
        Program& light = app->programs[app->lightsProgramIdx];
        app->texturedMeshProgramIdx_uPosition = glGetUniformLocation(light.handle, "uPositionTexture");
        app->texturedMeshProgramIdx_uNormals = glGetUniformLocation(light.handle, "uNormalsTexture");
        app->texturedMeshProgramIdx_uAlbedo = glGetUniformLocation(light.handle, "uAlbedoTexture");
        light.vertexInputLayout.attributes.push_back({ 0, 3 });
        light.vertexInputLayout.attributes.push_back({ 1, 2 });

        app->drawLightsProgramIdx = LoadProgram(app, "shaders.glsl", "DRAW_LIGHT");
        Program& texturedSphereLightProgram = app->programs[app->drawLightsProgramIdx];
        app->drawLightsProgramIdx_uLightColor = glGetUniformLocation(texturedSphereLightProgram.handle, "lightColor");
        app->drawLightsProgramIdx_uViewProjection = glGetUniformLocation(texturedSphereLightProgram.handle, "projectionView");
        app->drawLightsProgramIdx_uModel = glGetUniformLocation(texturedSphereLightProgram.handle, "model");
        texturedSphereLightProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
        break;
    }
    }

    app->cubemapTexture = loadCubeMap(app->cubeFaces);
    glUseProgram(app->cubeProgramIdx);
    glUniform1i(app->cubeProgramIdx, app->showCubeMap);
}

void InitBuffers(App* app)
{
    glGenTextures(1, &app->colorController);
    glBindTexture(GL_TEXTURE_2D, app->colorController);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->normalsController);
    glBindTexture(GL_TEXTURE_2D, app->normalsController);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthController);
    glBindTexture(GL_TEXTURE_2D, app->depthController);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->albedoController);
    glBindTexture(GL_TEXTURE_2D, app->albedoController);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->positionController);
    glBindTexture(GL_TEXTURE_2D, app->positionController);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->frameBufferController);
    glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferController);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorController, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsController, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->albedoController, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->positionController, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthController, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                      ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                    ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default:                                            ELOG("Unknown franebuffer status error | %i", framebufferStatus);
        }
    }

    glDrawBuffers(4, &app->colorController);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderCube()
{
	static unsigned int cubeVAO = 0;
	static unsigned int cubeVBO = 0;
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void RenderCubeMap(App* app)
{
    float cubeVertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //Render
    glBindVertexArray(cubeVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LEQUAL);
    glUseProgram(app->skyBoxProgramIdx);

    glBindVertexArray(skyVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

}

unsigned int loadCubeMap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;

    for (unsigned int i = 0; i < faces.size(); ++i)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void renderQuad()
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void RenderSphere()
{
	static unsigned int sphereVAO = 0;
	static unsigned int indexCount;

	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const int H = 32;
		const int V = 16;
		for (int h = 0; h <= H; ++h)
		{
			for (int v = 0; v < V + 1; ++v)
			{
				float nh = (float)h / (float)H;
				float nv = (float)v / (float)V;
				float xPos = std::cos(nh * 2.0f * PI) * std::sin(nv * PI);
				float yPos = std::cos(nv * PI);
				float zPos = std::sin(nh * 2.0f * PI) * std::sin(nv * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(nh, nv));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int h = 0; h < H; ++h)
		{
			if (!oddRow) 
			{
				for (unsigned int v = 0; v <= V; ++v)
				{
					indices.push_back(h * (V + 1) + v);
					indices.push_back((h + 1) * (V + 1) + v);
				}
			}
			else
			{
				for (int v = V; v >= 0; --v)
				{
					indices.push_back((h + 1) * (V + 1) + v);
					indices.push_back(h * (V + 1) + v);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = indices.size();

		std::vector<float> data;
		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}