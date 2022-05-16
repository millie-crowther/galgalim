#version 100

attribute vec4 vertexPosition;
attribute vec2 vertexTextureCoordinate;

uniform mat4 viewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 modelMatrix;

varying vec2 textureCoordinate;

void main() {
    textureCoordinate = vertexTextureCoordinate;
    gl_Position = uProjectionMatrix * viewMatrix * modelMatrix * vertexPosition;
}