#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace pgm
{
	float* readPGM(const char* fileName, int& width, int& height);
};
