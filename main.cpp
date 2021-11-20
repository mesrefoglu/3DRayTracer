#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

const float PI = 3.14159265359f;

struct Sphere {
    vec3 center;
    float radius;

    // constructor
    Sphere(const vec3 &c, const float &r) : center(c), radius(r) {}

    // determine if a ray from point orig with direction dir (normalized) intersect the sphere
    bool ray_intersect(const vec3 &orig, const vec3 &dir) const {
        vec3 dist = center - orig;								// distance b/w center of sphere and orig
        float projToOrigin = dist * dir;						// distance b/w the projection of the center on the ray and orig
        float d2 = dist * dist - projToOrigin * projToOrigin;	// sqr of the distance b/w ray and center
        if (d2 > radius * radius) return false;					// if the d2 is greater than radius, no intersection
        float projToIntersections = sqrt(radius * radius - d2);	// distance b/w intersections and projection
        float t0 = projToOrigin - projToIntersections;			// distance from origin to first intersection
        float t1 = projToOrigin + projToIntersections;			// distance from origin to second intersection
        if (t0 < 0) t0 = t1;									// this happens when ray is inside the sphere
        if (t0 < 0) return false;								// this happens when ray is in front of the sphere
        return true;
    }
};

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const Sphere &sphere) {
    if (!sphere.ray_intersect(orig, dir)) { // ray does not intersect with the sphere
        return vec3{0.2, 0.7, 0.8}; // background color
    }

    return vec3{0.4, 0.4, 0.3}; // "sphere" color
}

void render(const Sphere& sphere) {
    const int width = 1024;
    const int height = 768;
    const float fov = PI / 2.f; // field of view is 90 degrees (half pi)
    std::vector<vec3> framebuffer(width * height);

    #pragma omp parallel for
	  for (size_t i = 0; i < width; i++) {
        for (size_t j = 0; j < height; j++) {
            float x = (i + 0.5) -  width/2.;			// find x component of ray
            float y = -(j + 0.5) + height/2.;			// find y component of ray
            float z = width / (2. * tan(fov / 2.f));	// find z component of ray
            framebuffer[i + j * width] = cast_ray(vec3{0, 0, 0}, vec3{x, y, z}.normalize(), sphere);
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("./out.ppm", std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n"; // set he ppm file properties
    for (vec3& c : framebuffer) {
        ofs << (char)(int)(255.f * c[0]) // red
            << (char)(int)(255.f * c[1]) // green
            << (char)(int)(255.f * c[2]); // blue
    }
    ofs.close();
}

int main() {
	Sphere sphere(vec3{-3, 0, 16}, 2);
    render(sphere);
    return 0;
}