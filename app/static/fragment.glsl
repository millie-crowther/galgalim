varying lowp vec4 vColor;

uniform sampler2D colourTexture;

void main() {
    gl_FragColor = vColor;
}