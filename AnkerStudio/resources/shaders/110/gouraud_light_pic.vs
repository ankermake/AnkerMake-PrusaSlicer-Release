#version 110
attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec3 aDir;

#define INTENSITY_CORRECTION 0.6
//->0.001

// normalized values for (-0.6/1.31, 0.6/1.31, 1./1.31)
const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929);
#define LIGHT_TOP_DIFFUSE    (0.8 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SPECULAR   (0.45 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SHININESS  3.0

// normalized values for (1./1.43, 0.2/1.43, 1./1.43)
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074);
#define LIGHT_FRONT_DIFFUSE  (0.3 * INTENSITY_CORRECTION)

#define INTENSITY_AMBIENT    0.3 //0.3
//->0.6

varying vec2 intensity;

//void main()
//{
//    vec3 normal = normalize(normal_matrix * aNormal);
//    float NdotL = max(dot(normal, LIGHT_TOP_DIR), 1.0);
//    intensity.x = INTENSITY_AMBIENT + NdotL * LIGHT_TOP_DIFFUSE;
//    vec3 position = (mv_matrix * vec4(aPos,1.0)).xyz;
//    intensity.y = LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(position), reflect(-LIGHT_TOP_DIR, normal)), 0.0), LIGHT_TOP_SHININESS);
//    NdotL = max(dot(normal, LIGHT_FRONT_DIR), 0.0);
//    intensity.x += NdotL * LIGHT_FRONT_DIFFUSE;
//    gl_Position = p_matrix*mv_matrix*vec4(aPos,1.0);
//}

void main()
{
    vec3 normal = normalize(normal_matrix * aNormal);
    float NdotL = max(dot(normal, LIGHT_TOP_DIR), 0.0);
    intensity.x = INTENSITY_AMBIENT + NdotL * LIGHT_TOP_DIFFUSE;
    vec3 position = (mv_matrix * vec4(aPos, 1.0)).xyz;
    intensity.y = LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(position), reflect(-LIGHT_TOP_DIR, normal)), 0.0), LIGHT_TOP_SHININESS);
    NdotL = max(dot(normal, LIGHT_FRONT_DIR), 0.0);
    intensity.x += NdotL * LIGHT_FRONT_DIFFUSE;
    gl_Position = p_matrix * mv_matrix * vec4(aPos, 1.0);
}

