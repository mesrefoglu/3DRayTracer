#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

const float PI = 3.14159265359f;

struct Material {
    vec3 diffuse_color;

	// default constructor
    Material() : diffuse_color() {}

	// constructor with color
    Material(const vec3 &diffuse_c) : diffuse_color(diffuse_c) {}
};

struct Sphere {
    vec3 center;
    float radius;
	Material material;

    // constructor
    Sphere(const vec3 &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

    // determine if a ray from point orig with direction dir (normalized) intersect the sphere
    bool ray_intersect(const vec3 &orig, const vec3 &dir, float &t0) const {
        vec3 dist = center - orig;								// distance b/w center of sphere and orig
        float projToOrigin = dist * dir;						// distance b/w the projection of the center on the ray and orig
        float d2 = dist * dist - projToOrigin * projToOrigin;	// sqr of the distance b/w ray and center
        if (d2 > radius * radius) return false;					// if the d2 is greater than radius, no intersection
        float projToIntersections = sqrt(radius * radius - d2);	// distance b/w intersections and projection
        t0 = projToOrigin - projToIntersections;				// distance from origin to first intersection
        float t1 = projToOrigin + projToIntersections;			// distance from origin to second intersection
        if (t0 < 0) t0 = t1;									// this happens when ray is inside the sphere
        if (t0 < 0) return false;								// this happens when ray is in front of the sphere
        return true;
    }
};

// return true if a sphere hit the ray, false otherwise. mutate variables to show what is the last hit
bool scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
    float spheres_dist = std::numeric_limits<float>::max();	// the distance to the closest sphere
    for (size_t i=0; i < spheres.size(); i++) {
        float dist_i;
		// check if intersects and closer than the closest sphere so far
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i; 						// make this one closer
            hit = orig + dir * dist_i;					// the point ray hits the sphere
            N = (hit - spheres[i].center).normalize();	// the normalized direction towards the hit from center
            material = spheres[i].material;				// mutate material to show it later
        }
    }
    return spheres_dist < 1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres) {
    vec3 point, N;
    Material material;

    if (!scene_intersect(orig, dir, spheres, point, N, material)) {
        return vec3{0.4, 0.85, 1}; // background color
    }

    return material.diffuse_color; // "sphere" color
}

void render(const std::vector<Sphere> &spheres) {
    const int width = 1024;
    const int height = 768;
    const float hFOV = PI / 2.f; // horizontal field of view is 90 degrees (half pi)
    std::vector<vec3> framebuffer(width * height);

    #pragma omp parallel for
	  for (size_t i = 0; i < width; i++) {
        for (size_t j = 0; j < height; j++) {
            float x = (i + 0.5) -  width/2.;			// find x component of ray
            float y = -(j + 0.5) + height/2.;			// find y component of ray
            float z = width / (2. * tan(hFOV / 2.f));	// find z component of ray
            framebuffer[i + j * width] = cast_ray(vec3{0, 0, 0}, vec3{x, y, z}.normalize(), spheres);
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
	Material mat1(vec3{0.6, 0, 0.6});
	Material mat2(vec3{0, 0.6, 0.6});
	Material mat3(vec3{0.6, 0.6, 0});
	Material mat4(vec3{0.4, 0.4, 0.4});
	
	std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3{-3, 0, 16}, 2, mat1));
    spheres.push_back(Sphere(vec3{-1, -1.5, 12}, 2, mat2));
    spheres.push_back(Sphere(vec3{1.5, -0.5, 18}, 3, mat3));
    spheres.push_back(Sphere(vec3{7, 5, 18}, 4, mat4));

    render(spheres);
    return 0;
}