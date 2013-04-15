#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
__constant char hw[] = "Hello World\n\0";
__kernel void hello(__global char * out)
{
size_t tid = get_global_id(0);
out[tid] = hw[tid];
}

__kernel void color(__write_only image2d_t image)
{
	size_t row = get_global_id(0);
	size_t col = get_global_id(1);

	float4 c = (float4)(0.f, 1.f, 0.f, 1.f);
	int2 pos = (int2)(row, col);
	write_imagef(image, pos, c);
	//printf("pos = %v2d\ncolor = %2.2v4f\n", pos, c);
}