attribute vec3 aVertexPosition;
attribute vec3 aVertexNormal;
varying mediump vec3 color;
uniform mat4 uPMatrix;
uniform mat4 uMVMatrix;

void main(void) {
	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
	color = aVertexNormal;
}