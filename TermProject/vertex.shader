attribute vec3 aVertexPosition;
attribute vec3 aVertexNormal;
varying mediump vec3 color;
uniform mat4 uPMatrix;
uniform mat4 uVMatrix;
uniform mat4 uMMatrix;

void main(void) {
	gl_Position = uPMatrix * uVMatrix * uMMatrix * vec4(aVertexPosition, 1.0);
	color = aVertexNormal;
}