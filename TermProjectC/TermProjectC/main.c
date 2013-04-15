#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>

#define WINDOW_TITLE_PREFIX "Metaballs"

int width = 400,
	height = 300,
	WindowHandle = 0;

unsigned char pixels[400*300*4];
GLuint tex; 

cl_int err;
cl_platform_id platform_id[2];
cl_device_id device_id[3];
cl_context context;
cl_command_queue cQ;
cl_program program;
cl_kernel kernel;
cl_mem input, output;

void Initialize(int, char*[]);
void InitializeCL(void);
void InitBuffers();
void InitWindow(int, char*[]);
void CheckError(int, char* message);
void CreateProgram(void);
void ResizeFunction(int, int);
void RenderFunction(void);

int main(int argc, char* argv[])
{
	int size = 400*300*4;


	for(int i = 0; i < size; i+=4){
		pixels[i] = 0;
		pixels[i+1] = 255;
		pixels[i+2] = 255;
		pixels[i+3] = 255;
	}
	
	Initialize(argc, argv);
	//InitializeCL();
	glutMainLoop();
	
	system("PAUSE");
	exit(EXIT_SUCCESS);
}

void CheckError(int err, char* message)
{
	if (err != CL_SUCCESS)
    {
        printf("Error: %d @ %s\n", err, message);
    }
}

void CreateProgram()
{
	FILE* fin;
	size_t fSize, kernelSourceSize;
	char *buffer, *kernelSource;

	fin = fopen("kernels.cl", "rb");
	fseek(fin, 0, SEEK_END);
	fSize = ftell(fin);
	rewind(fin);

	buffer = (char*)malloc(sizeof(char)*fSize+1);
	buffer[fSize] = '\0';
	fread(buffer, 1, fSize, fin);
	fclose(fin);

	program = clCreateProgramWithSource(context, 1, (const char**) &buffer, &fSize, &err);
	CheckError(err, "Create Program with Source");
	free(buffer);
	/*clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, NULL, &kernelSourceSize);
    kernelSource = (char*) malloc(kernelSourceSize);
    clGetProgramInfo(program, CL_PROGRAM_SOURCE, kernelSourceSize, kernelSource, NULL);
    printf("\nKernel source:\n\n%s\n", kernelSource);
	free(kernelSource);*/


}

void InitializeCL()
{
	char out[13];
	char devinfo[200];
	cl_uint num;
	size_t work_dim = 1;
	const size_t global_work_size[] = {256, 256, 1};
	size_t ws = 256;

	err = clGetPlatformIDs(1, platform_id, NULL);
	CheckError(err, "GetPlatformIDs");
	//printf("%d\n", platform_id);
	err = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 1, device_id, &num);
	printf("Number of devices is %d\n", num);
	printf("Device id %d\n", device_id[0]);
	CheckError(err, "GetDeviceIDs");


	err = clGetDeviceInfo(device_id[0], CL_DEVICE_NAME, 200, devinfo, NULL);
	CheckError(err, "GetDeviceInfo");
	printf("%d %s \n", device_id, devinfo);

	cl_bool image_support;
	err = clGetDeviceInfo(device_id[0], CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL);
	CheckError(err, "GetDeviceInfo");
	printf("Device Image Support %d\n", image_support);

	cl_context_properties properties[7] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) platform_id[0],
		0};

	context = clCreateContext(properties, 1, device_id, NULL, NULL, &err);
	CheckError(err, "CreateContext");

	cQ = clCreateCommandQueue(context, device_id[0], NULL, &err);
	CheckError(err, "CreateCommandQueue");

	CreateProgram();

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	CheckError(err, "BuildProgram");

	kernel = clCreateKernel(program, "hello", &err);
	CheckError(err, "CreateKernel");

	InitBuffers();

	err = clSetKernelArg(kernel, 0, sizeof(char*), (void *) &output);
	CheckError(err, "SetKernelArg");


//	err = clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_WORK_GROUP_SIZE, sizeof(cl_uint), (void*) &global_work_size, &num);
//	CheckError(err, "GetKernelWorkGroupInfo");

	//printf("gws %d\n", global_work_size[0]);

	//if(cQ != NULL && kernel != NULL) printf("NOT NULL\n");
	err = clEnqueueNDRangeKernel(cQ, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL); 
	CheckError(err, "EnqueueNDRangeKernel");
	clFinish(cQ);
	err = clEnqueueReadBuffer(cQ, output, 1, 0, 13, out, NULL, NULL, NULL);
	CheckError(err, "EnqueueReadBuffer");
	printf("%s", out);

	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, 400, 300);
	
	//glBufferData(GL_ARRAY_BUFFER, 400, NULL, GL_STATIC_DRAW);
	//cl_mem vbo_buff = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, 2, &err);
	glFinish();

	cl_mem rbo_buf = clCreateFromGLRenderbuffer(context, CL_MEM_READ_WRITE, rbo, &err);
	CheckError(err, "CreateFromGLRenderbuffer");

	/*GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE0, tex);
	cl_mem tex_buf = clCreateFromGLTexture(context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, */
	err = clEnqueueAcquireGLObjects(cQ, 1, &rbo_buf, 0, NULL, NULL);
	CheckError(err, "Acquire GL Objects");

	cl_kernel colorKernel = clCreateKernel(program, "color", &err);
	CheckError(err, "Create Color Kernel");

	err = clSetKernelArg(colorKernel, 0, sizeof(cl_mem*), (void*) &rbo_buf);
	CheckError(err, "Set Color Kernel Arg 0");

	err = clEnqueueNDRangeKernel(cQ, colorKernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
	CheckError(err, "Enqueue Color Kernel");

	clFinish(cQ);
	
	err = clEnqueueReleaseGLObjects(cQ, 1, &rbo_buf, 0, NULL, NULL);
	CheckError(err, "Release GL Objects");

	clFinish(cQ);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	switch(glCheckFramebufferStatus(GL_FRAMEBUFFER))
    {

        case GL_FRAMEBUFFER_COMPLETE:    printf("The fbo is complete\n"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:    printf("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:    printf("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n"); break; 
    }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glReadPixels(0, 0, 300, 400, GL_RGBA, GL_RGBA32F, pixels);
	//memset(pixels, 0, sizeof(pixels));
	//glReadPixels(0, 0, 300, 400, GL_RGBA, GL_FLOAT, pixels);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glDrawPixels(300, 400, GL_RGBA, GL_FLOAT, pixels);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);
	//int x = 0;
	//glutSwapBuffers();
	//glutPostRedisplay();
	
	
}

void InitBuffers()
{
	//input = clCreateBuffer(context, CL_MEM_READ_ONLY, 
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 13, NULL, &err);
	CheckError(err, "CreateBuffer");
}

void Initialize(int argc, char* argv[])
{
	InitWindow(argc, argv);
	
	fprintf(
		stdout,
		"INFO: OpenGL Version: %s\n",
		glGetString(GL_VERSION)
	);

	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
}

void CreateTexture()
{
	int wrap = 1;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	// when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // if wrap is true, the texture wraps over at the edges (repeat)
    //       ... false, the texture ends at the edges (clamp)
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     wrap ? GL_REPEAT : GL_CLAMP );
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 400, 300, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void InitWindow(int argc, char* argv[])
{
	glutInit(&argc, argv);
	
	
	
	//glutInitContextVersion(4, 2);
	//glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	//glutInitContextProfile(GLUT_CORE_PROFILE);

	/*glutSetOption(
		GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
	);*/
	
	glutInitWindowSize(width, height);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	
	/*glEnable(GL_TEXTURE_2D);
	CreateTexture();*/

	WindowHandle = glutCreateWindow(WINDOW_TITLE_PREFIX);

	if(WindowHandle < 1) {
		fprintf(
			stderr,
			"ERROR: Could not create a new rendering window.\n"
		);
		exit(EXIT_FAILURE);
	}

	//glutReshapeFunc(ResizeFunction);
	glutDisplayFunc(RenderFunction);

	//glEnable(GL_DEPTH_TEST);

	GLenum error = glewInit();
	if(error != GLEW_OK)
		printf("GLEW not initialized with Error %s\n", glewGetErrorString(error));
}

void ResizeFunction(int Width, int Height)
{
	width = Width;
	height = Height;
	glViewport(0, 0, width, height);
}


void RenderFunction(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glFlush();
	glutSwapBuffers();
	//glRasterPos2i(0, 0);
	//glDrawPixels(400, 300, GL_RGBA, GL_UNSIGNED_BYTE, (void*) &pixels);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	/*glBindTexture(GL_TEXTURE_2D, tex);
	glBegin( GL_QUADS );
	glTexCoord2d(0.0,0.0); glVertex2d(0.0,0.0);
	glTexCoord2d(1.0,0.0); glVertex2d(1.0,0.0);
	glTexCoord2d(1.0,1.0); glVertex2d(1.0,1.0);
	glTexCoord2d(0.0,1.0); glVertex2d(0.0,1.0);
	glEnd();
	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();*/
}
