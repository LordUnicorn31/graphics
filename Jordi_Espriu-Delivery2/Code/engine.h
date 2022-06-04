//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>
#include "assimp_model_loading.h"
#include <map>

#include <glm/gtx/quaternion.hpp>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Buffer {
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void* data;
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8                                 stride;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    TEXTUREDQUAD,
    DEFERRED,
    FORWARD,
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32              vertexOffset;
    u32              indexOffset;
    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct Camera {
    float pitch = 0.f;
    float yaw = -90.f;

    float distanceToOrigin = 10.f;

	bool rotating = true;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);;

	glm::vec3 cameraRight = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);

	float fov = 60.f;

    glm::mat4 GetViewMatrix(const vec2& size) {
        
		float Phi = glm::radians(pitch);
		float Theta = glm::radians(yaw);

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        return glm::perspective(glm::radians(fov), size.x / size.y, 0.1f, 100.f) * view;
    }
};

struct Entity
{
    glm::mat4 matrix = glm::mat4(0.f);
    u32 modelId;
    u32 localParamsOffset;
    u32 localParamsSize;

    Entity(const glm::mat4& mat, u32 mdlId) : matrix(mat), modelId(mdlId) {};
};

enum LightType
{
    DIRECTIONAL,
    POINTT,
};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
    float intensity;

    Light(const LightType t, const vec3 c, vec3 dir, vec3 pos, float intensity) : type(t), color(c), direction(dir), position(pos), intensity(intensity) {}
};


struct App
{
    
    f32  deltaTime;
    bool isRunning;

    
    Input input;

    
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material>  materials;
    std::vector<Mesh>  meshes;
    std::vector<Model>  models;
    std::vector<Program>  programs;
    std::vector<Entity> entities;
    std::vector<Light> lights;
    

    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    u32 texturedMeshProgramForward;
    u32 meshProgramIdx;
    u32 lightsProgramIdx;
	u32 drawLightsProgramIdx;
    u32 cubeProgramIdx;
    u32 skyBoxProgramIdx;
    

    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;
    u32 model;
    u32 toyNormalTexIdx;
    u32 toyHeightTexIdx;
    u32 toyDiffuseTexIdx;
    

    Mode mode;
    
    
    GLuint embeddedVertices;
    GLuint embeddedElements;

    
    GLuint programUniformTexture;
    GLuint texturedMeshProgram_uTexture;

    GLuint texturedMeshProgramIdx_uAlbedo;
    GLuint texturedMeshProgramIdx_uPosition;
    GLuint texturedMeshProgramIdx_uNormals;
    GLuint texturedMeshProgramIdx_uDepth;
    GLuint texturedMeshProgramIdx_Deferred;
    GLuint texturedMeshProgramIdx_RelieveNormal;
    GLuint texturedMeshProgramIdx_RelieveHeight;
    GLuint texturedCube;

    std::vector<std::string> cubeFaces
    {
        "front.jpg",
        "back.jpg",
        "top.jpg",
        "bottom.jpg",
        "left.jpg",
        "right.jpg"
    };
    

    GLuint vao;
    GLuint frameBufferController;
    GLuint depthController;
    GLuint colorController;
    GLuint normalsController;
    GLuint albedoController;
    GLuint positionController;


	GLuint drawLightsProgramIdx_uLightColor;
	GLuint drawLightsProgramIdx_uViewProjection;
	GLuint drawLightsProgramIdx_uModel;


    Camera camera;
    Buffer cBuffer;
    GLuint globalParamsOffset;
    GLuint globalParamsSize;
    int uniformBlockAlignmentOffset;
	bool showGizmo = true;

    bool show = false;
    const GLubyte* version = nullptr;
    const GLubyte* renderer = nullptr;
    const GLubyte* vendor = nullptr;
    const GLubyte* shadingLanguageVersion = nullptr;
    const const unsigned char* extensions = nullptr;
    bool showRelief;
    bool showCubeMap;
    unsigned int cubemapTexture;
};

u32 LoadTexture2D(App* app, const char* filepath);

void Init(App* app);

void InitGPUInfo(App* app);

void InitBuffers(App* app);

void InitModes(App* app);

void CreateEntities(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void renderQuad();
void RenderSphere();
void RenderCube();
void RenderCubeMap(App* app);

unsigned int loadCubeMap(std::vector<std::string> faces);


