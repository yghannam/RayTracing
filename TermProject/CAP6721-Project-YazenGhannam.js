/*
Author: Yazen Ghannam
Spring 2013
CAP 6721
Project
*/

var glCtx = null;
var clCtx = null;
var shaderProgram = null;
var platforms = null;
var devices = [];
var canvas = null;
var canvasCtx = null;
var raster = null;
var rv = null;
var device = null;
var clCQ = null;
var clProgram = null;
var clKernels = new Object();
var clBuffers = new Object();
var numSpheres = 500;
var spherePositions = [];
var startDraw = false;
function parseShader(shaderFile){
	var xhttp = new XMLHttpRequest();
	xhttp.overrideMimeType('text/plain');
	xhttp.open("GET", shaderFile, false);
	xhttp.send();
	return xhttp.responseText;
}

function initShaders(){

	var vertexShaderCode = parseShader("vertex.shader");
	var vertexShader = glCtx.createShader(glCtx.VERTEX_SHADER);
	glCtx.shaderSource(vertexShader, vertexShaderCode);
	glCtx.compileShader(vertexShader);
	
	if (!glCtx.getShaderParameter(vertexShader, glCtx.COMPILE_STATUS)) {
            alert("Vertex Shader compilation failed: " +
				glCtx.getShaderInfoLog(vertexShader));
    }
	
	var fragmentShaderCode = parseShader("fragment.shader");
	var fragmentShader = glCtx.createShader(glCtx.FRAGMENT_SHADER);
	glCtx.shaderSource(fragmentShader, fragmentShaderCode);
	glCtx.compileShader(fragmentShader);
	
	if (!glCtx.getShaderParameter(fragmentShader, glCtx.COMPILE_STATUS)) {
            alert("Fragment Shader compilation failed: " +
				glCtx.getShaderInfoLog(fragmentShader));
    }
	
	shaderProgram = glCtx.createProgram();
	glCtx.attachShader(shaderProgram, vertexShader);
	glCtx.attachShader(shaderProgram, fragmentShader);
	glCtx.linkProgram(shaderProgram);
	if(!glCtx.getProgramParameter(shaderProgram, glCtx.LINK_STATUS)){
		alert("Failed to link shaders: " + glCtx.getProgramInfoLog(shaderProgram));
	}
	shaderProgram.positionBuffer = glCtx.createBuffer();
	glCtx.bindBuffer(glCtx.ARRAY_BUFFER, shaderProgram.positionBuffer);
	var positionLocation = glCtx.getAttribLocation(shaderProgram, "aVertexPosition");
	glCtx.enableVertexAttribArray(positionLocation);
	glCtx.vertexAttribPointer(positionLocation, 3, glCtx.FLOAT, false, 0, 0);
	
	shaderProgram.normalBuffer = glCtx.createBuffer();
	glCtx.bindBuffer(glCtx.ARRAY_BUFFER, shaderProgram.normalBuffer);
	var normalLocation = glCtx.getAttribLocation(shaderProgram, "aVertexNormal");
	glCtx.enableVertexAttribArray(normalLocation);
	glCtx.vertexAttribPointer(normalLocation, 3, glCtx.FLOAT, false, 0, 0);
	
	shaderProgram.elementBuffer = glCtx.createBuffer();
	
	shaderProgram.pMatrixLocation 				= glCtx.getUniformLocation(shaderProgram, "uPMatrix");
	shaderProgram.vMatrixLocation 				= glCtx.getUniformLocation(shaderProgram, "uVMatrix");
	shaderProgram.mMatrixLocation 				= glCtx.getUniformLocation(shaderProgram, "uMMatrix");
	// shaderProgram.normalMatrixLocation 			= glCtx.getUniformLocation(shaderProgram, "normalMatrix");
	// shaderProgram.cameraPositionLocation 		= glCtx.getUniformLocation(shaderProgram, "cameraPosition");
	// shaderProgram.nkLocation 					= glCtx.getUniformLocation(shaderProgram, "nk");
	// shaderProgram.rgbMatrixLocation				= glCtx.getUniformLocation(shaderProgram, "rgbMatrix");
	// shaderProgram.gammaLocation					= glCtx.getUniformLocation(shaderProgram, "gamma");
	// shaderProgram.cieLocation 					= glCtx.getUniformLocation(shaderProgram, "cie");
	// shaderProgram.texSamplerLocation			= glCtx.getUniformLocation(shaderProgram, "texSampler");
	// shaderProgram.colorTemperatureLocation		= glCtx.getUniformLocation(shaderProgram, "colorTemperature");
	// shaderProgram.enableCubeLocation			= glCtx.getUniformLocation(shaderProgram, "enableCube");
	// shaderProgram.enableHDRLocation				= glCtx.getUniformLocation(shaderProgram, "enableHDR");

}

function setSpherePositions(){
	if(spherePositions.length == 0)
		for(var i = 0; i < numSpheres; i++){
			var pos = [(Math.random()-0.5)/0.5 * 50, (Math.random()-0.5)/0.5 * 50, (Math.random()-0.5)/0.5 * 50];
			spherePositions.push(pos);
		}
	else
		for(var i = 0; i < numSpheres; i++){
			var pos = [(Math.random()-0.5)/0.5 * 50, (Math.random()-0.5)/0.5 * 50, (Math.random()-0.5)/0.5 * 50];
			spherePositions[i] = pos;
	}

}

function draw(){

	glCtx.clear(glCtx.COLOR_BUFFER_BIT | glCtx.DEPTH_BUFFER_BIT);
	
	for(var i = 0; i < numSpheres; i++){
		var mMatrix = mat4.create();
		mat4.translate(mMatrix, mMatrix, spherePositions[i]);
		//console.log(spherePositions[i]);
		
		glCtx.uniformMatrix4fv(shaderProgram.mMatrixLocation, false, mMatrix);
		glCtx.drawElements(glCtx.TRIANGLES, vertexIndexBuffer.length, glCtx.UNSIGNED_SHORT, 0);	
		
	}
}

var vertexPositionBuffer = [];
var vertexNormalBuffer = [];
var moonVertexTextureCoordBuffer;
var vertexIndexBuffer = [];
function initBuffers() {
	var latitudeBands = 30;
	var longitudeBands = 30;
	var radius = 1;
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

        vertexNormalBuffer.push(x);
        vertexNormalBuffer.push(y);
        vertexNormalBuffer.push(z);
        textureCoordData.push(u);
        textureCoordData.push(v);
        vertexPositionBuffer.push(radius * x);
        vertexPositionBuffer.push(radius * y);
        vertexPositionBuffer.push(radius * z);
      }
    }
	var indexData = [];
    for (var latNumber = 0; latNumber < latitudeBands; latNumber++) {
      for (var longNumber = 0; longNumber < longitudeBands; longNumber++) {
        var first = (latNumber * (longitudeBands + 1)) + longNumber;
        var second = first + longitudeBands + 1;
        vertexIndexBuffer.push(first);
        vertexIndexBuffer.push(second);
        vertexIndexBuffer.push(first + 1);

        vertexIndexBuffer.push(second);
        vertexIndexBuffer.push(second + 1);
        vertexIndexBuffer.push(first + 1);
      }
    }
	
	var pMatrix = mat4.create();
	var vMatrix = mat4.create();

	mat4.perspective(pMatrix, 45, 1, 0.1, 100.0);
	mat4.lookAt(vMatrix, [0, 0, -60], [0, 0, 0], [0, 1, 0]);
	

	glCtx.uniformMatrix4fv(shaderProgram.pMatrixLocation, false, pMatrix);
    glCtx.uniformMatrix4fv(shaderProgram.vMatrixLocation, false, vMatrix);
		
	//console.log(vertexPositionBuffer);
	
	glCtx.bindBuffer(glCtx.ARRAY_BUFFER, shaderProgram.positionBuffer);
	glCtx.bufferData(glCtx.ARRAY_BUFFER, new Float32Array(vertexPositionBuffer), glCtx.STATIC_DRAW);
	
	glCtx.bindBuffer(glCtx.ARRAY_BUFFER, shaderProgram.normalBuffer);
	glCtx.bufferData(glCtx.ARRAY_BUFFER, new Float32Array(vertexNormalBuffer), glCtx.STATIC_DRAW);	
	
	glCtx.bindBuffer(glCtx.ELEMENT_ARRAY_BUFFER, shaderProgram.elementBuffer);
	glCtx.bufferData(glCtx.ELEMENT_ARRAY_BUFFER,  new Uint16Array(vertexIndexBuffer), glCtx.STATIC_DRAW);
}

function initWebGL(){	
	try{
		glCtx = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
		//alert("WebGL initialized successfully");
		glCtx.canvas.width = 500; //document.body.offsetWidth;
		glCtx.canvas.height = 500; //document.body.offsetHeight;
		glCtx.clearColor(0.0, 0.0, 0.0, 1.0);	
		glCtx.enable(glCtx.DEPTH_TEST);
		glCtx.depthFunc(glCtx.LEQUAL);
	}
	catch(e){
		alert("Unable to use WebGL");
	}
}

function initWebCL(){
	platforms = WebCL.getPlatformIDs();
	for ( var i in platforms ){
		devices[i] = platforms[i].getDeviceIDs(WebCL.CL_DEVICE_TYPE_DEFAULT);
	}
	//console.log(platforms);
	//console.log(devices);
	device = devices[0][0];
	clCtx = WebCL.createContext([WebCL.CL_CONTEXT_PLATFORM, platforms[0]], devices[0]);
	clCQ = clCtx.createCommandQueue(devices[0][0], 0);
}

function main(){
	canvas = document.getElementById("myCanvas");
	initWebGL();
	initWebCL();
	
	//console.log(canvas);
	//var imageData = clCtx.createImage2D(WebCL.CL_MEM_READ_WRITE, WebCL.CL_RGBA, 500, 500, 500*4);
	console.log(clCtx.getSupportedImageFormats(WebCL.CL_MEM_READ_WRITE, WebCL.CL_MEM_OBJECT_IMAGE2D));
	
	//alert("main()");
	
	initShaders();	
	glCtx.useProgram(shaderProgram);
	
	initBuffers();
	
	var drawTime = new Date().getTime();
	var requestAnimationFrame = window.webkitRequestAnimationFrame || window.mozRequestAnimationFrame;
	(function callback(){
		requestAnimationFrame(callback);
		if(startDraw){
			setSpherePositions();
			drawTime = new Date().getTime();
			draw();
			console.log(new Date().getTime() - drawTime +"ms");
		}
	})();
	//alert("drawScene()");
}