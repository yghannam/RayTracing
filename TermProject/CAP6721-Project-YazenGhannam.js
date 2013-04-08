/*
Author: Yazen Ghannam
Spring 2013
CAP 6721
Project
*/

var gl = null;
var shaderProgram = null;

function parseShader(shaderFile){
	var xhttp = new XMLHttpRequest();
	xhttp.overrideMimeType('text/plain');
	xhttp.open("GET", shaderFile, false);
	xhttp.send();
	return xhttp.responseText;
}

function initShaders(){

	var vertexShaderCode = parseShader("vertex.shader");
	var vertexShader = gl.createShader(gl.VERTEX_SHADER);
	gl.shaderSource(vertexShader, vertexShaderCode);
	gl.compileShader(vertexShader);
	
	if (!gl.getShaderParameter(vertexShader, gl.COMPILE_STATUS)) {
            alert("Vertex Shader compilation failed: " +
				gl.getShaderInfoLog(vertexShader));
    }
	
	var fragmentShaderCode = parseShader("fragment.shader");
	var fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
	gl.shaderSource(fragmentShader, fragmentShaderCode);
	gl.compileShader(fragmentShader);
	
	if (!gl.getShaderParameter(fragmentShader, gl.COMPILE_STATUS)) {
            alert("Fragment Shader compilation failed: " +
				gl.getShaderInfoLog(fragmentShader));
    }
	
	shaderProgram = gl.createProgram();
	gl.attachShader(shaderProgram, vertexShader);
	gl.attachShader(shaderProgram, fragmentShader);
	gl.linkProgram(shaderProgram);
	if(!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)){
		alert("Failed to link shaders: " + gl.getProgramInfoLog(shaderProgram));
	}
	shaderProgram.positionBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, shaderProgram.positionBuffer);
	var positionLocation = gl.getAttribLocation(shaderProgram, "aVertexPosition");
	gl.enableVertexAttribArray(positionLocation);
	gl.vertexAttribPointer(positionLocation, 3, gl.FLOAT, false, 0, 0);
	
	shaderProgram.normalBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, shaderProgram.normalBuffer);
	var normalLocation = gl.getAttribLocation(shaderProgram, "aVertexNormal");
	gl.enableVertexAttribArray(normalLocation);
	gl.vertexAttribPointer(normalLocation, 3, gl.FLOAT, false, 0, 0);
	
	shaderProgram.elementBuffer = gl.createBuffer();
	
	shaderProgram.pMatrixLocation 				= gl.getUniformLocation(shaderProgram, "uPMatrix");
	shaderProgram.mvMatrixLocation 				= gl.getUniformLocation(shaderProgram, "uMVMatrix");
	//shaderProgram.mMatrixLocation 				= gl.getUniformLocation(shaderProgram, "uMMatrix");
	// shaderProgram.normalMatrixLocation 			= gl.getUniformLocation(shaderProgram, "normalMatrix");
	// shaderProgram.cameraPositionLocation 		= gl.getUniformLocation(shaderProgram, "cameraPosition");
	// shaderProgram.nkLocation 					= gl.getUniformLocation(shaderProgram, "nk");
	// shaderProgram.rgbMatrixLocation				= gl.getUniformLocation(shaderProgram, "rgbMatrix");
	// shaderProgram.gammaLocation					= gl.getUniformLocation(shaderProgram, "gamma");
	// shaderProgram.cieLocation 					= gl.getUniformLocation(shaderProgram, "cie");
	// shaderProgram.texSamplerLocation			= gl.getUniformLocation(shaderProgram, "texSampler");
	// shaderProgram.colorTemperatureLocation		= gl.getUniformLocation(shaderProgram, "colorTemperature");
	// shaderProgram.enableCubeLocation			= gl.getUniformLocation(shaderProgram, "enableCube");
	// shaderProgram.enableHDRLocation				= gl.getUniformLocation(shaderProgram, "enableHDR");

}

function draw(){

	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
	
	var pMatrix = mat4.create();
	var mvMatrix = mat4.create();

	mat4.perspective(pMatrix, 45, 1, 0.1, 100.0);
	
	mat4.lookAt(mvMatrix, [0, 0, -6], [0, 0, 0], [0, 1, 0]);

	gl.uniformMatrix4fv(shaderProgram.pMatrixLocation, false, pMatrix);
    gl.uniformMatrix4fv(shaderProgram.mvMatrixLocation, false, mvMatrix);
		
	//console.log(moonVertexPositionBuffer);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, shaderProgram.positionBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(moonVertexPositionBuffer), gl.STATIC_DRAW);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, shaderProgram.normalBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(moonVertexNormalBuffer), gl.STATIC_DRAW);	
	
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, shaderProgram.elementBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,  new Uint16Array(moonVertexIndexBuffer), gl.STATIC_DRAW);
	
	gl.drawElements(gl.TRIANGLES, moonVertexIndexBuffer.length, gl.UNSIGNED_SHORT, 0);	
}

var moonVertexPositionBuffer = [];
var moonVertexNormalBuffer = [];
var moonVertexTextureCoordBuffer;
var moonVertexIndexBuffer = [];
function initBuffers() {
	var latitudeBands = 30;
	var longitudeBands = 30;
	var radius = 2;
	var vertexPositionData = [];
    var normalData = [];
    var textureCoordData = [];
    for (var latNumber = 0; latNumber <= latitudeBands; latNumber++) {
      var theta = latNumber * Math.PI / latitudeBands;
      var sinTheta = Math.sin(theta);
      var cosTheta = Math.cos(theta);

      for (var longNumber = 0; longNumber <= longitudeBands; longNumber++) {
        var phi = longNumber * 2 * Math.PI / longitudeBands;
        var sinPhi = Math.sin(phi);
        var cosPhi = Math.cos(phi);

        var x = cosPhi * sinTheta;
        var y = cosTheta;
        var z = sinPhi * sinTheta;
        var u = 1 - (longNumber / longitudeBands);
        var v = 1 - (latNumber / latitudeBands);

        moonVertexNormalBuffer.push(x);
        moonVertexNormalBuffer.push(y);
        moonVertexNormalBuffer.push(z);
        textureCoordData.push(u);
        textureCoordData.push(v);
        moonVertexPositionBuffer.push(radius * x);
        moonVertexPositionBuffer.push(radius * y);
        moonVertexPositionBuffer.push(radius * z);
      }
    }
	var indexData = [];
    for (var latNumber = 0; latNumber < latitudeBands; latNumber++) {
      for (var longNumber = 0; longNumber < longitudeBands; longNumber++) {
        var first = (latNumber * (longitudeBands + 1)) + longNumber;
        var second = first + longitudeBands + 1;
        moonVertexIndexBuffer.push(first);
        moonVertexIndexBuffer.push(second);
        moonVertexIndexBuffer.push(first + 1);

        moonVertexIndexBuffer.push(second);
        moonVertexIndexBuffer.push(second + 1);
        moonVertexIndexBuffer.push(first + 1);
      }
    }
}

function initWebGL(canvas){
	var context = null;
	
	try{
		context = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
		//alert("WebGL initialized successfully");
	}
	catch(e){
		alert("Unable to use WebGL");
	}
	return context;
}

function main(){
	var canvas = document.getElementById("myCanvas");
	gl = initWebGL(canvas);
	gl.canvas.width=500; //document.body.offsetWidth;
	gl.canvas.height=500; //document.body.offsetHeight;
	gl.clearColor(0.0, 0.0, 0.0, 1.0);	
	gl.enable(gl.DEPTH_TEST);
	gl.depthFunc(gl.LEQUAL);
	
	//alert("main()");
	
	initShaders();	
	gl.useProgram(shaderProgram);
	
	initBuffers();
	draw();
	//alert("drawScene()");
}