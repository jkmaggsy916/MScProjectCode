#include "pgm.h"

float* pgm::readPGM(const char* fileName, int& width, int& height)
{	
	std::ifstream in(fileName, ios::in);
	if (!in) {
		cout << "error" << endl;
	}
	int c, offset = 1;
	int w, h, max;
	for (int f = 0; f < offset; f++) { in.ignore(256, '\n'); }
	in >> c; w = c;
    width = w;
	in >> c; h = c;
    height = h;
	in >> c; max = c;//max
	for (int f = 0; f < offset; f++) { in.ignore(256, '\n'); }
	int size = w * h; //calculate the number of pixels in the image
	float* pixels = new float[size]; //allocate array for pixels
	for (int i = 0; i < size; i++) {
		int value;
		in >> c; value = c;
        //all values are between -1 and 1
		pixels[i] = 2.0f*(float)value/(float)max - 1.0f;
	}
	in.close();
	return pixels;
}
