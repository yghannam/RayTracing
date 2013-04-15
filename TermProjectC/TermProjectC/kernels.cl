#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
__constant char hw[] = "Hello World\n\0";
__kernel void hello(__global char * out)
{
size_t tid = get_global_id(0);
out[tid] = hw[tid];
}

__kernel void color(__global uchar4 *pixel)
{
	const int col = get_global_id(0);
	const int row = get_global_id(1);
	int index =  row * 400 + col;
	pixel[index].xyz = 128;
	pixel[index].w = 255;
	//printf("pixel = %v4d\n", pixel[index]);
}