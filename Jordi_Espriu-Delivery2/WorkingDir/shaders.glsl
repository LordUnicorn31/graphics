///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main() {
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
     	 float 			intensity;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
 	int 			uLightCount;
 	Light			uLight[16];
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vNormals;
out vec3 vViewDir;
out vec3 vPosition;

void main() {
    gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);
    vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
    vTexCoord = aTexCoord;
    vViewDir = uCameraPosition - aPosition;
    vPosition = vec3(uWorldMatrix * vec4(aPosition,1.0));
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

 struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
     float 			intensity;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
	int 			uLightCount;
	Light			uLight[16];
};

in vec2 vTexCoord;
in vec3 vNormals;
in vec3 vViewDir;
in vec3 vPosition;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;

vec3 DirectionalLight(vec3 lightPosition, vec3 color, vec3 normal);
vec3 PointLight(vec3 lightPosition, vec3 color, vec3 normal, vec3 fragPosition, vec3 view_dir, vec2 texCoords);

void main() {
	vec3 lightsColors = vec3(0.0,0.0,0.0);
	for(int i = 0; i < uLightCount; ++i)
	{		if(uLight[i].type == 0) //Directional
			    lightsColors += DirectionalLight(uLight[i].position, uLight[i].color, normalize(vNormals));
            else //PointLight
            {
                lightsColors += PointLight(uLight[i].position, uLight[i].color, vNormals, vPosition,normalize(vViewDir), vTexCoord);
            }
	}
	oColor 		= vec4(lightsColors, 1.0)*texture(uTexture, vTexCoord);
	oNormals 	= vec4(vNormals, 1.0);
    gl_FragDepth = gl_FragCoord.z - 0.2;
    oAlbedo   =   texture(uTexture, vTexCoord);
}

vec3 DirectionalLight(vec3 lightPos, vec3 color, vec3 normal){
    vec3 lightColor = vec3(1.);
    // Ambient
    vec3 ambient = lightColor * 0.15 * color;

    // Diffuse
    vec3 lightDirection = normalize(-lightPos);
    float diffuseIntensity = max(dot(normal, lightDirection),0.0);
    vec3 diffuse = diffuseIntensity * lightColor * color;

    // Specular
    float specularStrength = .85;
    float specularIntensity = pow(max(dot(normal, lightDirection),0.0),32.);
    vec3 specular = specularStrength * specularIntensity * lightColor;
    
    return ambient + diffuse + specular;
}

vec3 PointLight(vec3 lightPos, vec3 color, vec3 normal, vec3 fragPosition, vec3 view_dir, vec2 texCoords)
{
    // Ambient
    vec3 ambient = color;

    // Diffuse
    vec3 lightDir = normalize(lightPos - fragPosition);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;

     // Specular
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), 0.0) * 0.01;
    vec3 specular = ambient * spec;

    // Range
    float distance = length(lightPos - fragPosition);
    float range = 4/distance;
    
	return (diffuse + specular) * range;
}
#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangents;
layout(location=4) in vec3 aBiTangents;

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
 	int 			uLightCount;
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vNormals;
out vec3 vViewDir;
out vec3 vPosition;
out mat3 TBN;
out mat3 worldViewMatrix;

void main() {
    gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);
    vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
    vTexCoord = aTexCoord;
    vViewDir = uCameraPosition - aPosition;
    vPosition = vec3(uWorldMatrix * vec4(aPosition,1.0));
    worldViewMatrix = mat3(uWorldMatrix);
    vec3 T = normalize(vec3(uWorldMatrix * vec4(aTangents,   0.0)));
    vec3 B = normalize(vec3(uWorldMatrix * vec4(aBiTangents, 0.0)));
    vec3 N = normalize(vec3(uWorldMatrix * vec4(vNormals,    0.0)));
    TBN = mat3(T,B,N);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

 struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
     float 			intensity;
};

vec2 reliefMapping(vec2 texCoords, vec3 viewDir);

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
	int 			uLightCount;
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

in vec2 vTexCoord;
in vec3 vNormals;
in vec3 vViewDir;
in vec3 vPosition;
in mat3 TBN;
in mat3 worldViewMatrix;

uniform sampler2D uAlbedoTexture;

uniform unsigned int uNormalMapping;
uniform sampler2D uNormalTexture;

uniform unsigned int uBumpMapping;
uniform sampler2D uBumpTexture;
uniform int uShowRelief;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oPosition;
void main() {
    vec3 normals = vec3(0.0);
    vec2 tCoords = vTexCoord;

    if(uShowRelief == 1)
    {
        tCoords = reliefMapping(tCoords, vViewDir);
        normals = texture(uNormalTexture, vTexCoord).rgb;
        normals = normals * 2.0 - 1.0;
        normals = normalize(inverse(transpose(TBN)) * normals);
    }


	oColor 		= texture(uAlbedoTexture, tCoords);
	oNormals 	= vec4(normals, 1.0);
    
    oAlbedo   =   texture(uAlbedoTexture, tCoords);
    oPosition = vec4(transpose(TBN)*vPosition, 1.0);
    gl_FragDepth = gl_FragCoord.z - 0.2;
}

vec2 reliefMapping(vec2 texCoords, vec3 viewDir)
{
	int numSteps = 25;

	// Compute the view ray in texture space
	vec3 rayTexspace = transpose(TBN) * inverse(worldViewMatrix) * viewDir;

	// Increment
	vec3 rayIncrementTexspace;
	rayIncrementTexspace.xy = rayTexspace.xy / abs(rayTexspace.z * textureSize(uBumpTexture,0).x);
	rayIncrementTexspace.z = 1.0/numSteps;

	// Sampling state
	vec3 samplePositionTexspace = vec3(texCoords, 0.0);
	float sampledDepth = 1.0 - texture(uBumpTexture, samplePositionTexspace.xy).r;

	// Linear search
	for (int i = 0; i < numSteps && samplePositionTexspace.z < sampledDepth; ++i)
	{
		samplePositionTexspace += rayIncrementTexspace;
		sampledDepth = 1.0 - texture(uBumpTexture, samplePositionTexspace.xy).r;
	}

    // get depth after and before collision for linear interpolation
    float afterDepth  = samplePositionTexspace.z - sampledDepth;
    float beforeDepth = texture(uBumpTexture, samplePositionTexspace.xy).r - samplePositionTexspace.z + sampledDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = samplePositionTexspace.xy * weight + samplePositionTexspace.xy * (1.0 - weight);

    return finalTexCoords;
}


#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
     float 	intensity;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
 	int 			uLightCount;
 	Light			uLight[16];
};

out vec2 vTexCoord;

void main() {

	gl_Position = vec4(aPosition, 1.0);

	vTexCoord = aTexCoord;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

 struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
     float 	intensity;
};

vec3 DirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords){
    vec3 lightColor = vec3(1.);
    // Ambient
    vec3 ambient = lightColor * 0.15 * light.color;

    // Diffuse
    vec3 lightDirection = normalize(-light.position);
    float diffuseIntensity = max(dot(normal, light.direction),0.0);
    vec3 diffuse = diffuseIntensity * lightColor * light.color;

    // Specular
    float specularStrength = 0.01;
    float specularIntensity = pow(max(dot(normal, lightDirection),0.0),0.1);
    vec3 specular = specularStrength * specularIntensity * lightColor * light.intensity;
    
    return (ambient + diffuse + specular) * light.intensity;
}

vec3 PointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords)
{
    vec3 ambient = light.color;

    vec3 lightDir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;

    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), 0.0) * 0.01;
    vec3 specular = ambient * spec;

    float distance = length(light.position - frag_pos);
    float range = 1/distance;      
	return (diffuse + specular) * range * light.intensity;
}

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPosition;
	int 			uLightCount;
	Light			uLight[16];
};

uniform sampler2D uPositionTexture;
uniform sampler2D uNormalsTexture;
uniform sampler2D uAlbedoTexture;

in vec2 vTexCoord;

layout(location = 0) out vec4 oColor;

void main() {
	vec3 fragPos = texture(uPositionTexture, vTexCoord).rgb;
	vec3 norms = texture(uNormalsTexture, vTexCoord).rgb;
	vec3 diffuseCol = texture(uAlbedoTexture, vTexCoord).rgb;

	vec3 viewDir = normalize(uCameraPosition - fragPos);
	vec3 lightsColors = vec3(0.0,0.0,0.0);
	for(int i = 0; i < uLightCount; ++i)
	{		
        if(uLight[i].type == 0) //Directional
        {
			lightsColors += DirectionalLight(uLight[i], norms, normalize(viewDir), vTexCoord);
        }
        else //PointLight
        {
            lightsColors += PointLight(uLight[i], norms, fragPos, viewDir, vTexCoord);
        }
	}
    oColor = vec4(lightsColors + diffuseCol * 0.2, 1.0);
}
#endif
#endif

#ifdef DRAW_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;

uniform mat4 projectionView;
uniform mat4 model;

void main() {
	gl_Position = projectionView * model * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oColor;

uniform vec3 lightColor;

void main() {
	oColor = vec4(lightColor, 1.0);
    gl_FragDepth = gl_FragCoord.z - 0.2;
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_CUBE

#if defined(VERTEX) ///////////////////////////////////////////////////
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Position = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{    
    vec3 I = normalize(Position - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}

#endif
#endif


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SHOW_SKY
#if defined(VERTEX) ///////////////////////////////////////////////////
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}

#endif
#endif
