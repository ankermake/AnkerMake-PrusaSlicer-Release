#version 110
varying vec2 intensity;
uniform vec4 uniform_color;
void main()
{
    gl_FragColor = vec4(vec3(intensity.y, intensity.y, intensity.y) + uniform_color.rgb * intensity.x, 1.0);
}
