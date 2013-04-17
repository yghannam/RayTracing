#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

#define EPSILON 0.01f
#define DEBUG 0
//#define NUMSHAPES 3
//#define ACCELERATION 1
//#define MAXDIM 10
#define RATE 0.1f
#define CORERADIUS 1

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

	if(ACCELERATION == 1)
	{
		bool hasIntersection = false;
		// Intersect with all shapes
		float tArray[2*NUMSHAPES];
		int shapeIndexArray[2*NUMSHAPES];
		bool entryArray[2*NUMSHAPES];

		int i = 0;
		for (i = 0; i < numShapes; i++)
		{
			Sphere sphere;
			sphere.c = shapeData[i].xyz;
			sphere.r = 2*CORERADIUS;

			float rv = raySphere(ray, sphere, false);
			tArray[2*i] = rv > EPSILON ? rv : INFINITY;
			shapeIndexArray[2*i] = i;
			entryArray[2*i] = true;

			rv = raySphere(ray, sphere, true);
			tArray[2*i+1] = rv > EPSILON ? rv : INFINITY;
			shapeIndexArray[2*i+1] = i;
			entryArray[2*i+1] = false;

			if(!hasIntersection)
				hasIntersection = tArray[2*i] != INFINITY || tArray[2*i+1] != INFINITY; 

		}
		
		// Return if no intersection
		if(!hasIntersection)
			return intersection;

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

		/*printf("\n");
		for(i = 0; i < 2*numShapes; i++)
		{
			printf("%x\n", entryArray[i]);
		}
*/
		// Step along ray
		bool activeArray[NUMSHAPES];
		float densities[NUMSHAPES];
		float3 diffs[NUMSHAPES];
		int x;
		for(x = 0; x < numShapes; x++)
		{
			activeArray[x] = true;
		}
		
		//activeArray[shapeIndexArray[0]] = entryArray[0];
		for(i = 1; i < 2*numShapes; i++)
		{

			if(tArray[i] == INFINITY)
				break;

			//activeArray[shapeIndexArray[i]] = entryArray[i];			

			float t;
			for(t = tArray[i-1]; t < tArray[i]; t += 0.1f)
			{
				// Calculate density from active list
				float total_density = 0.f;
				float3 pos = ray.o + t * ray.d;
				for(j = 0; j < numShapes; j++)
				{			
					if(activeArray[j])
					{
						diffs[j] = pos - shapeData[j].xyz;
						float r = length(diffs[j]);
						densities[j] = 1.f/(r*r);
						total_density += densities[j];
					}
				}

				
				if(total_density >= CORERADIUS)
				{
					/*printf("\n");
					for(x = 0; x < numShapes; x++)
					{
						printf("%x\n", activeArray[x]);
					}	*/
					intersection.N = (float3)(0.f, 0.f, 0.f);
					for(j = 0; j < numShapes; j++)
					{			
						if(activeArray[j])
						{
							intersection.N += densities[j] * normalize(diffs[j]);
						}
					}
					float invDensity = 1.f / total_density;
					intersection.N *= invDensity;
					intersection.t = t;
					i = 2*numShapes;
					break;
				}					
			}
		}

	}

	else if(ACCELERATION == 2)
	{

	}

	else
	{
		intersection.shapeIndex = -1;
		intersection.density = 0.f;
		float density = 0.f;
	
		int i, j;
	
		float densities[NUMSHAPES];
		float3 diffs[NUMSHAPES];
		float t = 0.f;
		for ( j = 0; j < 2000; j++)
		{
			float total_density = 0.f;
			float3 pos = ray.o + t*ray.d;

			for( i = 0; i < numShapes; i++)
			{
				diffs[i] = pos - shapeData[i].xyz;
				float r = length(diffs[i]);
				densities[i] = 1.f/(r*r);
				total_density += densities[i];
			}
			

			if(total_density > CORERADIUS)
			{ 
				intersection.N = (float3)(0.f, 0.f, 0.f);
				for(i = 0; i < numShapes; i++)
				{			
					intersection.N += densities[i] * normalize(diffs[i]);
				}
				float invDensity = 1.f / total_density;
				intersection.N *= invDensity;
				intersection.t = t;
				break;
			}	
			t += 0.01f;
		}
	}
	
	
	return intersection;
}

__kernel void getPixelColor(
	const int numShapes,
	__global float4 *shapeData,
	__write_only image2d_t raster){
	
		const int col = get_global_id(0);
		const int row = get_global_id(1);
		int columns = 400;
		int rows = 300;
		float width = 1.f;
		float height = width * rows / columns;

		float3 eye = (float3)(0.f, 0.f, -12.f);
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
			float3 Intensity = clamp(dot(intersection.N, normalize(lightDir)), 0.f, 1.f) + clamp(dot(intersection.N, normalize(-lightDir)), 0.f, 1.f);
			color = (float4)(Intensity, 1.f);
		}
	
		write_imagef(raster, (int2)(col, row), color);
}

__kernel void moveShapes(
	const int numShapes,
	__global float4 *shapeData,
	__global float4 *shapeVector)
{
	int i = get_global_id(0);

	if(i < numShapes)
	{

		if(shapeData[i].x >= MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(-1.f, 0.f, 0.f), shapeVector[i].xyz);
		}
		else if(shapeData[i].x <= -MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(1.f, 0.f, 0.f), shapeVector[i].xyz);
		}
		else if(shapeData[i].y >= MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(0.f, -1.f, 0.f), shapeVector[i].xyz);
		}
		else if(shapeData[i].y <= -MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(0.f, 1.f, 0.f), shapeVector[i].xyz);
		}
		else if(shapeData[i].z >= MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(0.f, 0.f, -1.f), shapeVector[i].xyz);
		}
		else if(shapeData[i].z <= -MAXDIM)
		{
			shapeVector[i].xyz = reflect( (float3)(0.f, 0.f, 1.f), shapeVector[i].xyz);
		}

		shapeData[i] += RATE * shapeVector[i];		
	}
}