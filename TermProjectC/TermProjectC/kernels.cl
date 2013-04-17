#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

#define Pi 3.14159265358979323846f
#define EPSILON 0.01
#define DEBUG 0
#define NUMSHAPES 3
#define ACCELERATED 1

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


typedef struct {
	float3 d;
	float3 o;
}Ray;

typedef struct{
	float3 c;
	float r;
}Sphere;

typedef struct{
	float t;
	int shapeIndex;
	float3 P;
	float3 N;
	float density;
}Intersection;


float rayPlane(
	Ray ray, 
	float3 normal,
	float3 point){
		float t = INFINITY;		
		float rayNormal = dot(ray.d, normal);
		if( rayNormal != 0){
			t = dot(point-ray.o, normal)/rayNormal;
		}		
		return (t>0) ? t : INFINITY;
	}
	
float3 rayTriangle(
	Ray ray, 
	float3 p0,
	float3 p1,
	float3 p2){
	float t = INFINITY;
	
	/*
	float3 normal = cross(p1-p0, p2-p0);
	t = rayPlane(rayOrigin, rayDirection, normal, p0);
	if(t > 0){
		float3 p = rayOrigin + t*rayDirection;
		float normalSquared = dot(normal, normal);
		float alpha = dot(normal, cross(p-p0, p2-p0))/normalSquared;
		float beta = dot(normal, cross(p1-p0, p-p0))/normalSquared;
		
		if(alpha < 0 || beta < 0 || alpha+beta > 1){
t = 0;
		}
	}
	*/
	float3 e1 = p1-p0;
	float3 e2 = p2-p0;
	float3 s = ray.o-p0;
	float invDet = 1.0f / dot(cross(ray.d, e2), e1);
	t = invDet * dot(cross(s, e1), e2);
	float alpha = invDet * dot(cross(ray.d, e2), s);
	float beta = invDet * dot(cross(s, e1), ray.d);
	
	if(t < 0 || alpha < 0 || beta < 0 || alpha+beta > 1){
		t = INFINITY;
	}
	
	return (float3)(t, alpha, beta);
}

float raySphere(
	Ray ray, 
	Sphere sphere,
	bool second){
	float t = INFINITY;
	float a = dot(ray.d, ray.d);
	float b = 2.0f * dot(ray.o - sphere.c ,ray.d);
	float c = dot(ray.o-sphere.c,ray.o-sphere.c) - sphere.r*sphere.r;
	float discriminant = b*b - 4.0f*a*c;
	if (discriminant == 0){
		t = -b/(2.0f*a);
	}
	else{
		float root = sqrt(discriminant);
		float q = (b < 0) ? (-0.5f*(b-root)) : (-0.5f*(b+root));
		float x1 = q/a;
		float x2 = c/q;
		if(x1 > 0 && x2 > 0){
			if(second)
				return (x1 > x2) ? x1:x2;
			return (x1 < x2) ? x1:x2;
		}
		else if (x1 > 0 && x2 <= 0){
			return x1;
		}
		else if (x1 <= 0 && x2 > 0){
			return x2;
		}
		else{
			return INFINITY;
		}
	}
	return t;
}

float3 reflect(float3 normal, float3 incident){

	return  2 * normal * clamp(dot(normalize(incident), normal), 0.0f, 1.f) - normalize(incident);

}

float3 transformRayDirection(float3 localDirection, float3 U, float3 V, float3 W){
	float3 world;

	world.x = U.x*localDirection.x + V.x*localDirection.y + W.x*localDirection.z;
	world.y = U.y*localDirection.x + V.y*localDirection.y + W.y*localDirection.z;
	world.z = U.z*localDirection.x + V.z*localDirection.y + W.z*localDirection.z;
	return world;
}

Intersection intersect(
	Ray ray,
	int numShapes, 
	__global float4 *shapeData){
	

	Intersection intersection;
	intersection.t = INFINITY;	

	if(ACCELERATED)
	{

		// Intersect with all shapes
		float tArray[2*NUMSHAPES];
		int shapeIndexArray[2*NUMSHAPES];
		int entryArray[2*NUMSHAPES];

		int i = 0;
		for (i = 0; i < numShapes; i++)
		{
			Sphere sphere;
			sphere.c = shapeData[i].xyz;
			sphere.r = 2;

			float rv = raySphere(ray, sphere, false);
			tArray[2*i] = rv > EPSILON ? rv : INFINITY;
			shapeIndexArray[2*i] = i;
			entryArray[2*i] = 1;

			/*if(rv != INFINITY)
				printf("rv: %f\n", rv);*/

			rv = raySphere(ray, sphere, true);
			tArray[2*i+1] = rv > EPSILON ? rv : INFINITY;
			shapeIndexArray[2*i+1] = i;
			entryArray[2*i+1] = 0;

			/*if(rv != INFINITY)
				printf("rv: %f\n", rv);*/
			//printf("%x\n", i);
		}
		//tArray[5] = 5.f;
	

		// Sort intersection by t (distance)
		// Using Selection Sort, for now
		int j, iMin;

		for(j = 0; j < 2*numShapes-1; j++)
		{
			iMin = j;

			for( i = j+1; i < 2*numShapes; i++)
			{
				if(tArray[i] < tArray[iMin])
				{
					iMin = i;
				}
			}

			if(iMin != j)
			{
				float tempT = tArray[iMin];
				tArray[iMin] = tArray[j];
				tArray[j] = tempT;

				int tempS = shapeIndexArray[iMin];
				shapeIndexArray[iMin] = shapeIndexArray[j];
				shapeIndexArray[j] = tempS;

				bool tempB = entryArray[iMin];
				entryArray[iMin] = entryArray[j];
				entryArray[j] = tempB;
			}

		}
		/*if(tArray[3] > 0.f)
			printf("%f %f %f %f %f %f\n", tArray[0], tArray[1], tArray[2], tArray[3], tArray[4], tArray[5]);*/

		// Step along ray
		

		bool activeArray[NUMSHAPES];
		activeArray[shapeIndexArray[0]] = entryArray[0] == 1 ? true:false;
		for(i = 1; i < 2*numShapes; i++)
		{

			if(tArray[i] == INFINITY)
				break;

			activeArray[shapeIndexArray[i]] = entryArray[i] == 1 ? true:false;
		
			float t;
			for(t = tArray[i-1]; t < tArray[i]; t += 0.05f)
			{
				//if(t == INFINITY) printf("%d\n", i);

				// Calculate density from active list
				float total_density = 0.f;
				float3 pos = ray.o + t * ray.d;
				intersection.N = (float3)(0.f, 0.f, 0.f);
				for(j = 0; j < numShapes; j++)
				{			
					//if(activeArray[j])
					//{
						float3 diff = pos - shapeData[j].xyz;
						float r = length(diff);
						float density = 1.f/(r*r);
						total_density += density;
						intersection.N += density * normalize(diff);
					//}
				}

				intersection.N.x /= total_density;
				intersection.N.y /= total_density;
				intersection.N.z /= total_density;
				//printf("j: %d t: %2.2f density: %2.2f\n", j, t, density);
				if(total_density > 1.f)
				{ 
					//printf("%d %v3f \n", j, intersection.P);
					//printf("j: %d t: %f density: %f\n", j, t, density);
					intersection.t = t;
					i = 2*numShapes;
					break;
				}	
				
				//t += 0.01f;
			}
		}

	}
	else
	{
		intersection.shapeIndex = -1;
		intersection.density = 0.f;
		float density = 0.f;
	
		int i, j;
	
		float t = 0.f;
		for ( j = 0; j < 2000; j++)
		{
			float total_density = 0.f;
			intersection.P = ray.o + t*ray.d;
			intersection.N = (float3)(0.f, 0.f, 0.f);

			for( i = 0; i < numShapes; i++)
			{
				/*Sphere sphere;
				sphere.c = shapeData[i].xyz;
				sphere.r = 2.f;	*/			
					
				float3 diff = intersection.P - shapeData[i].xyz;
				float r = length(diff);
				float density = 1.f/(r*r);
				total_density += density;
				intersection.N += density * normalize(diff);

				
				intersection.density = total_density ;
			
			}
			intersection.N.x /= total_density;
			intersection.N.y /= total_density;
			intersection.N.z /= total_density;
			//printf("j: %d t: %2.2f density: %2.2f\n", j, t, density);
			if(total_density > 0.7f)
			{ 
				//printf("%d %v3f \n", j, intersection.P);
				//printf("j: %d t: %f density: %f\n", j, t, density);
				
				break;
			}	
			intersection.t = t;
			t += 0.01f;
		}

		/*if(density < 0.5f)
			intersection.t = INFINITY;*/
	}
	
	
	return intersection;
}

__kernel void clearRaster(
	const int columns,
	__global uchar4 *raster){
		const int col = get_global_id(0);
		const int row = get_global_id(1);
		raster[row * columns + col] = (uchar4)(0,0,0,255);
	}

__kernel void getPixelColor(
	const int numShapes,
	__global float4 *shapeData,
	__write_only image2d_t raster){
	
		/*if(DEBUG)
		{
			printf("Number of shapes: %d\n", numShapes);
			printf("Shape data: %2.2v4f %2.2v4f\n", shapeData[0], shapeData[1]);
			return;
		}*/

		const int col = get_global_id(0);
		const int row = get_global_id(1);
		int columns = 400;
		int rows = 300;
		float width = 1.f;
		float height = width * rows / columns;

		float3 eye = (float3)(0.f, 0.f, -10.f);
		float3 W = normalize(eye);
		float3 up = (float3)(0.f, 1.f, 0.f);
		float3 U = normalize(cross(up, W));
		float3 V = cross(W, U);

		float3 localDirection;
		localDirection.x = width * ( (0.5f+col)/columns - 0.5f);
		localDirection.y = height * (0.5f - (0.5f+row)/rows);
		localDirection.z = -1.f;		

		Ray worldRay;
		worldRay.o = eye;
		worldRay.d = transformRayDirection(localDirection, U, V, W);

		float4 color = (float4)(0.f, 128.f, 0.f, 255.f);

		float3 lightDir = (float3)(-1.f, -1.f, -1.f);
	
		Ray ray;
		ray.o = worldRay.o;
		ray.d = worldRay.d;
		
		Intersection intersection;	


		intersection 
			= intersect(ray, numShapes, shapeData);	

		if (intersection.t < INFINITY)
		{				
			//printf("reflect: %2.2f\n", intersection.reflect);

			float3 Intensity = clamp(dot(intersection.N, normalize(lightDir)), 0.f, 1.f) + clamp(dot(intersection.N, normalize(-lightDir)), 0.f, 1.f);
/*
			float red = round(255.f*Intensity.x);
			float green = round(255.f*Intensity.y);
			float blue = round(255.f*Intensity.z);*/
			color = (float4)(Intensity, 1.f);
		}
	
		//printf("Color %v4d\n", color);
		write_imagef(raster, (int2)(col, row), color);
		//raster[row * columns + col] = color;		
}

__kernel void moveShapes(
	const int numShapes,
	__global float4 *shapeData)
{
	int i = get_global_id(0);

	if(i < numShapes)
	{
		if(i%2 == 0)
		{
			if(shapeData[i].w == 1.f)
				shapeData[i].x += 0.1f;
			else
				shapeData[i].x -= 0.1f;
		}
		else if(i > 0 && i%2 == 0)
		{
			if(shapeData[i].w == -1.f)
				shapeData[i].y += 0.1f;
			else
				shapeData[i].y -= 0.1f;
		}
		else
		{
			if(shapeData[i].w == -1.f)
				shapeData[i].x += 0.1f;
			else
				shapeData[i].x -= 0.1f;
		}

		if(shapeData[i].x > 3.f || shapeData[i].x < -3.f || shapeData[i].y > 3.f)
			shapeData[i].w *= -1.f;
	}
}