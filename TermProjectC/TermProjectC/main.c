#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <CL/cl.h>

#define WINDOW_TITLE_PREFIX "Chapter 1"

int CurrentWidth = 800,
	CurrentHeight = 600,
	WindowHandle = 0;

cl_int err;
cl_platform_id platform_id;
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
void CheckError(int);
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

	program = clCreateProgramWithSource(context, 1, (const char**) &buffer, &fSize, NULL);
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
	const size_t global_work_size[] = {256, 1, 1};
	size_t ws = 256;

	err = clGetPlatformIDs(1, &platform_id, NULL);
	CheckError(err, "GetPlatformIDs");
	//printf("%d\n", platform_id);
	err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, device_id, &num);
	printf("Number of devices is %d\n", num);
	printf("Device id %d\n", device_id[0]);
	CheckError(err, "GetDeviceIDs");


	err = clGetDeviceInfo(device_id[0], CL_DEVICE_NAME, 200, devinfo, NULL);
	CheckError(err, "GetDeviceInfo");
	printf("%d %s \n", device_id, devinfo);

	context = clCreateContext(NULL, 1, device_id, NULL, NULL, &err);
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
	
	glutInitContextVersion(4, 2);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutSetOption(
		GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
	);
	
	glutInitWindowSize(CurrentWidth, CurrentHeight);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	WindowHandle = glutCreateWindow(WINDOW_TITLE_PREFIX);

	if(WindowHandle < 1) {
		fprintf(
			stderr,
			"ERROR: Could not create a new rendering window.\n"
		);
		exit(EXIT_FAILURE);
	}

	glutReshapeFunc(ResizeFunction);
	glutDisplayFunc(RenderFunction);
}

void ResizeFunction(int Width, int Height)
{
	CurrentWidth = Width;
	CurrentHeight = Height;
	glViewport(0, 0, CurrentWidth, CurrentHeight);
}

void RenderFunction(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glutSwapBuffers();
	glutPostRedisplay();
}
