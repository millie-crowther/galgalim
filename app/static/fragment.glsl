#version 100

precision highp float;

uniform sampler2D colourTexture;
uniform sampler2D normalTexture;

varying vec2 textureCoordinate;

vec4 srgbToLinear(vec4 srgbIn) {
    return vec4(pow(srgbIn.xyz, vec3(2.2)), srgbIn.w);
}

void main() {
    gl_FragColor = srgbToLinear(texture2D(colourTexture, textureCoordinate));
}