#version 100

attribute vec4 vertexPosition;
attribute vec2 vertexTextureCoordinate;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

varying vec2 textureCoordinate;

void main() {
    textureCoordinate = vertexTextureCoordinate;
    gl_Position = uProjectionMatrix * uModelViewMatrix * vertexPosition;
}