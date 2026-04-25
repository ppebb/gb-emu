#pragma once

#include <GLFW/glfw3.h>

void input_init(GLFWwindow *window);
uint8_t input_read();
void input_poll();
