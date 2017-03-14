
#include <GLFW/glfw3.h>
#include <cstdio>

int main (int argc, const char** argv) {
    if (!glfwInit())
        return fprintf(stderr, "Could not initialize glfw\n"), -1;
    return 0;
}
