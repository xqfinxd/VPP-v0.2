#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (set = 0, binding = 0) buffer Numbers {
    int data[];
} numbers[3];

void main() {
    // Get the index of the current invocation
    uint index = 0;

    // Read the input value at this index
    int input_ = numbers[0].data[index];

    // Do some computation with it
    int output_ = input_ * 2 + 1;

    // Write it back to the buffer
    numbers[1].data[index] = output_;
}