#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight pointLight;
uniform Material material;

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayVec = normalize(viewDir + lightDir);
        float spec = pow(max(dot(Normal, halfwayVec), 0.0), material.shininess);

        // attenuation
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        // combine results


        vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        if (diff != 0.0f) {
            specular = vec3(0.0f);
        }
        return (ambient + diffuse + specular);
}

float near = 0.2005f;
float far = 50.0f;

float linearizeDepth(float depth) {
    return (2.0 * far * near) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

float logDepth(float depth, float steepness, float offset) {

    float zVal = linearizeDepth(depth);
    return (1 / (1 + exp(-steepness * (zVal - offset))));
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcPointLight(pointLight, normal, FragPos, viewDir);
    float depth = logDepth(gl_FragCoord.z, 0.1f, 0.2f);

    FragColor = vec4(result, 1.0f) * (1.0 - depth) + vec4(depth * vec4(vec3(0.0085, 0.0085, 0.0090), 1.0));
    // FragColor = vec4(vec3(linerizeDepth(gl_FragCoord.z) / far), 1.0f);
    // FragColor = vec4(result, 1.0); //* vec4(vec3(1.0) - vec3(linearizeDepth(gl_FragCoord.z) / far), 1.0f);
}