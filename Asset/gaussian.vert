#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0, rgba8) uniform image2D inputImage;
layout (binding = 1, rgba8) uniform image2D outputImage;

void main() {
    ivec2 pixelCoords = ivec2(0,0);

    vec4 pixel = imageLoad(inputImage, pixelCoords);

    vec4 blurredPixel = vec4(0.0);

    // Apply a 5x5 Gaussian blur
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            // Get the pixel offset by the kernel
            vec4 neighbor = imageLoad(inputImage, pixelCoords + ivec2(x, y));

            // Gaussian kernel values from https://learnopengl.com/Advanced-Lighting/Bloom
            float kernel[5] = float[](1.0 / 16.0, 4.0 / 16.0, 6.0 / 16.0, 4.0 / 16.0, 1.0 / 16.0);

            // Multiply the neighbor by the corresponding kernel value
            neighbor *= kernel[x + 2] * kernel[y + 2];

            // Add the neighbor to the blurred pixel
            blurredPixel += neighbor;
        }
    }

    // Write the blurred pixel to the output image
    imageStore(outputImage, pixelCoords, blurredPixel);
}