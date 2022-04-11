attribute vec4 vertexPosition;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

varying lowp vec4 vColor;

void main() {
    gl_Position = uProjectionMatrix * uModelViewMatrix * vertexPosition;
    vColor = vec4(1.0, 1.0, 1.0, 1.0);
}