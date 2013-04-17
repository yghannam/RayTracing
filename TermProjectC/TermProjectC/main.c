#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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
cl_kernel colorKernel;
cl_kernel moveKernel;
cl_uint work_group_size;
size_t local_work_size[2];
size_t global_work_size[2];
cl_mem pixel_buf;

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
	Initialize(argc, argv);
	InitializeCL();
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
	

	err = clGetPlatformIDs(1, platform_id, NULL);
	CheckError(err, "GetPlatformIDs");
	//printf("%d\n", platform_id);
	err = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 1, device_id, &num);
	//printf("Number of devices is %d\n", num);
	//printf("Device id %d\n", device_id[0]);
	CheckError(err, "GetDeviceIDs");


	//err = clGetDeviceInfo(device_id[0], CL_DEVICE_NAME, 200, devinfo, NULL);
	//CheckError(err, "GetDeviceInfo");
	//printf("%d %s \n", device_id, devinfo);

	/*cl_uint info;
	err = clGetDeviceInfo(device_id[0], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &info, NULL);
	CheckError(err, "GetDeviceInfo");*/
	//printf("Device Image Support %d\n", image_support);

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

	/*kernel = clCreateKernel(program, "hello", &err);
	CheckError(err, "CreateKernel");*/

	InitBuffers();

	/*err = clSetKernelArg(kernel, 0, sizeof(char*), (void *) &output);
	CheckError(err, "SetKernelArg");
*/
	colorKernel = clCreateKernel(program, "getPixelColor", &err);
	CheckError(err, "Create Color Kernel");

	moveKernel = clCreateKernel(program, "moveShapes", &err);
	CheckError(err, "Create Move Kernel");

	
	err = clGetKernelWorkGroupInfo(colorKernel, device_id[0], CL_KERNEL_WORK_GROUP_SIZE, sizeof(cl_uint), (void*) &work_group_size, &num);
	CheckError(err, "GetKernelWorkGroupInfo");

	cl_uint xSize = (cl_uint)floor(sqrt(1.0*work_group_size));
	cl_uint ySize = (cl_uint)floor(1.0*work_group_size/xSize);
	local_work_size[0] = xSize;
	local_work_size[1] = ySize;
	global_work_size[0] = (cl_uint)ceil(1.0*width/local_work_size[0])*local_work_size[0];
	global_work_size[1] = (cl_uint)ceil(1.0*height/local_work_size[1])*local_work_size[1];
	//printf("gws %d\n", global_work_size[0]);

	//if(cQ != NULL && kernel != NULL) printf("NOT NULL\n");
	/*err = clEnqueueNDRangeKernel(cQ, kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL); 
	CheckError(err, "EnqueueNDRangeKernel");

	clFinish(cQ);

	err = clEnqueueReadBuffer(cQ, output, 1, 0, 13, out, NULL, NULL, NULL);
	CheckError(err, "EnqueueReadBuffer");
	printf("%s", out);*/

	
	/*err = clEnqueueAcquireGLObjects(cQ, 1, &rbo_buf, 0, NULL, NULL);
	CheckError(err, "Acquire GL Objects");*/

	int numShapes = 3;
	float shapeData[] = {1.5, 0.0, 0.0, 1.0, -1.5, 0.0, 0.0, 1.0, 0, 1.5, 0.0, 1.0};
	cl_mem shape_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, numShapes*4*4, NULL, &err);
	CheckError(err, "Create Shape Buffer");

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

    // Set parameters 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	pixel_buf = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, tex, &err); //clCreateBuffer(context, CL_MEM_READ_WRITE, width*height*4, NULL, &err);
	CheckError(err, "Create Pixel Buffer");

	/*cl_image_format info;
	err = clGetImageInfo(pixel_buf, CL_IMAGE_FORMAT, sizeof(cl_image_format), &info, NULL);
	CheckError(err, "Get Image Info");*/

	err = clSetKernelArg(colorKernel, 0, sizeof(cl_int), &numShapes);
	CheckError(err, "Set Color Kernel Arg 0");
	err = clSetKernelArg(colorKernel, 1, sizeof(cl_mem*), (void*) &shape_buf);
	CheckError(err, "Set Color Kernel Arg 1");
	err = clSetKernelArg(colorKernel, 2, sizeof(cl_mem), &pixel_buf);
	CheckError(err, "Set Color Kernel Arg 2");

	err = clSetKernelArg(moveKernel, 0, sizeof(cl_int), &numShapes);
	CheckError(err, "Set Move Kernel Arg 0");
	err = clSetKernelArg(moveKernel, 1, sizeof(cl_mem*), (void*) &shape_buf);
	CheckError(err, "Set Move Kernel Arg 1");

	err = clEnqueueWriteBuffer(cQ, shape_buf, 1, 0, numShapes*4*4, shapeData, 0, NULL, NULL);
	CheckError(err, "Write Shape Data");

	
	/*err = clEnqueueReleaseGLObjects(cQ, 1, &rbo_buf, 0, NULL, NULL);
	CheckError(err, "Release GL Objects");

	clFinish(cQ);*/

	
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

void InitWindow(int argc, char* argv[])
{
	glutInit(&argc, argv);
	
	glutInitWindowSize(width, height);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);	

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

	GLenum error = glewInit();
	if(error != GLEW_OK)
		printf("GLEW not initialized with Error %s\n", glewGetErrorString(error));

	glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        60.0,
        (GLfloat)width / (GLfloat)height,
        0.1,
        10.0);
}

void ResizeFunction(int Width, int Height)
{
	width = Width;
	height = Height;
	glViewport(0, 0, width, height);
}


void RenderFunction(void)
{
	glFinish();

	err = clEnqueueAcquireGLObjects(cQ, 1, &pixel_buf, 0, NULL, NULL);
	CheckError(err, "Acquire GL Objects");
	
	err = clEnqueueNDRangeKernel(cQ, colorKernel, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	CheckError(err, "Enqueue Color Kernel");

	clFinish(cQ);

	err = clEnqueueNDRangeKernel(cQ, moveKernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	CheckError(err, "Enqueue Move Kernel");

	clFinish(cQ);

	/*err = clEnqueueReadBuffer(cQ, pixel_buf, 1, 0, width*height*4, pixels, 0, NULL, NULL);
	CheckError(err, "Read Pixel Buffer");

	clFinish(cQ);
*/
	err = clEnqueueReleaseGLObjects(cQ, 1, &pixel_buf, 0, NULL, NULL);
	CheckError(err, "Release GL Objects");

	//glClear(GL_COLOR_BUFFER_BIT);
	
	// Bind  texture
    glBindTexture(GL_TEXTURE_2D, tex);

	// Display image using texture
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode( GL_MODELVIEW);
	glLoadIdentity();

	glViewport(0, 0, width, height);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0, -1.0, 0.5);

	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0, -1.0, 0.5);

	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0, 1.0, 0.5);

	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0, 1.0, 0.5);

	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	//glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    //glFlush();
	//SwapBuffers();
	glutSwapBuffers();
	glutPostRedisplay();
}
