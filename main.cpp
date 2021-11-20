#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

void render() {
    const int height = 768;
    const int width = 1024;
    std::vector<vec3> framebuffer(width * height);

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j <width; j++) {
            // each x value of framebuffer is a float between 0 and 1 referring to the height, and y is width
            framebuffer[i * width + j] = vec3{i / float(height), j / float(width), 0};
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("./out.ppm", std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n"; // set he ppm file properties
    for (vec3& c : framebuffer) {
        ofs << (char)(int)(255.f * c[0]) // red
            << (char)0 // green
            << (char)(int)(255.f * c[1]); // blue
    }
    ofs.close();
}

int main() {
    render();
    return 0;
}