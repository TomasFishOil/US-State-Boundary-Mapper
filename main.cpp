#include <iostream>
#include <fstream>
#include <cmath>
#include "windows.h"

using namespace std;

#define IMAGE_SIZE 256

void drawLine(char bitMap[][IMAGE_SIZE], int, int, int, int);

int main() {

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	char colorTable[1024];

	//Initialize the color table with grayscale values
	for (int i = 0; i < 256; i++) {
		colorTable[i * 4 + 0] = i;  // Blue
		colorTable[i * 4 + 1] = i;  // Green
		colorTable[i * 4 + 2] = i;  // Red
		colorTable[i * 4 + 3] = 0;  // Reserved
	}

	//The following defines the array which holds the image.  
	char bits[IMAGE_SIZE][IMAGE_SIZE] = { 0 };

	//Define and open the output file. 
	ofstream bmpOut("Temp/foo.bmp", ios::out | ios::binary);

	if (!bmpOut) {
		cout << "...could not open file, ending.";
		return -1;
	}

	//Initialize the bit map file header with static values.
	bmfh.bfType = 0x4d42;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih) + sizeof(colorTable);
	bmfh.bfSize = bmfh.bfOffBits + sizeof(bits);

	//Initialize the bit map information header with static values.
	bmih.biSize = 40;
	bmih.biWidth = IMAGE_SIZE;
	bmih.biHeight = -IMAGE_SIZE;
	bmih.biPlanes = 1;
	bmih.biBitCount = 8;
	bmih.biCompression = 0;
	bmih.biSizeImage = IMAGE_SIZE * IMAGE_SIZE;
	bmih.biXPelsPerMeter = 2835;  
	bmih.biYPelsPerMeter = 2835;
	bmih.biClrUsed = 256;
	bmih.biClrImportant = 0;

	float inputStateCode;
	float fileStateCode;
	int validPoints = 0;
	float longitudeMax;
	float longitudeMin;
	float latitudeMax;
	float latitudeMin;
	float longitudeDifference;
	float latitudeDifference;
	float onePixelSize;
	bool stateCodeFlag = false;

	//Receiving state code from user
	cout << "Please enter state code: ";
	cin >> inputStateCode;
	while (inputStateCode > 56 || inputStateCode < 1) {
		cout << "Error: State code must be in acceptable range 1-56: ";
		cin >> inputStateCode;
	}

	//Opening file with state info
	ifstream stateBoundary("Temp/lower48BoundaryFile.txt");
	if (!stateBoundary) {
		cout << "Error: File not opened, ending program.";
		return 0;
	}

	//Finding and saving count of points for given state code
	while (stateBoundary >> fileStateCode) {
		if (fileStateCode == inputStateCode) {
			stateBoundary >> validPoints;
			stateCodeFlag = true;
			break;
		}
	}

	if (!stateCodeFlag) {
		cout << "Error: State code not found, ending program.";
		return 0;
	}

	//Parallel arrays to hold paired longitudes and latitudes
	float* longitudes = new float[validPoints];
	float* latitudes = new float[validPoints];

	//Placing data in array
	for (int i = 0; i < validPoints; i++) {
		stateBoundary >> longitudes[i];
		stateBoundary >> latitudes[i];
		longitudes[i] = abs(longitudes[i]);
	}

	stateBoundary.close();

	longitudeMax = longitudes[0];
	longitudeMin = longitudes[0];
	latitudeMax = latitudes[0];
	latitudeMin = latitudes[0];
	for (int i = 0; i < validPoints; i++) {
		if (longitudes[i] > longitudeMax)
			longitudeMax = longitudes[i];
		if (longitudes[i] < longitudeMin)
			longitudeMin = longitudes[i];
		if (latitudes[i] > latitudeMax)
			latitudeMax = latitudes[i];
		if (latitudes[i] < latitudeMin)
			latitudeMin = latitudes[i];
	}

	__asm {
		FLD longitudeMax
		FLD longitudeMin
		FSUBP st(1), st(0)
		FSTP longitudeDifference

		FLD latitudeMax
		FLD latitudeMin
		FSUBP st(1), st(0)
		FSTP latitudeDifference

		FLD longitudeDifference
		FLD latitudeDifference
		FCOMI st(0), st(1)
		JAE findOnePixelSize
		FXCH st(1)

		findOnePixelSize:
			MOV EBX, IMAGE_SIZE - 1
			PUSH EBX
			FILD dword ptr[esp]
			ADD esp, 4
			FDIVP st(1), st(0)
			FSTP onePixelSize
	}

	//Drawing each point
	for (int i = 0; i < validPoints - 1; i++) {
		//Holding values from array for assembly calculations
		float longitude0 = longitudes[i];
		float latitude0 = latitudes[i];
		float longitude1 = longitudes[i + 1];
		float latitude1 = latitudes[i + 1];
		//Points to be drawn
		int x0;
		int y0;
		int x1; 
		int y1;

		//Finding first x coordinate
		__asm {
			FLD longitude0
			FSUB longitudeMin
			FLD onePixelSize
			FDIVP st(1), st(0)
			FISTP x0
		}
		x0 = IMAGE_SIZE - 1 - x0;

		//Finding first y coordinate
		__asm {
			FLD latitude0
			FSUB latitudeMin
			FLD onePixelSize
			FDIVP st(1), st(0)
			FISTP y0
		}
		y0 = IMAGE_SIZE - 1 - y0;

		//Finding second x coordinate
		__asm {
			FLD longitude1
			FSUB longitudeMin
			FLD onePixelSize
			FDIVP st(1), st(0)
			FISTP x1
		}
		x1 = IMAGE_SIZE - 1 - x1;

		//
		__asm {
			FLD latitude1
			FSUB latitudeMin
			FLD onePixelSize
			FDIVP st(1), st(0)
			FISTP y1
		}
		y1 = IMAGE_SIZE - 1 - y1;

		//Drawing line connecting two calculated points
		drawLine(bits, x0, y0, x1, y1);
	}

	//Writing out bitmap 
	char* workPtr;
	workPtr = (char*)&bmfh;
	bmpOut.write(workPtr, 14);
	workPtr = (char*)&bmih;
	bmpOut.write(workPtr, 40);
	workPtr = &colorTable[0];
	bmpOut.write(workPtr, sizeof(colorTable));
	workPtr = &bits[0][0];
	bmpOut.write(workPtr, IMAGE_SIZE * IMAGE_SIZE);
	bmpOut.close();

	delete[] longitudes;
	delete[] latitudes;

	//Showing finished job in paint
	system("mspaint \"Temp\\foo.bmp\"");

	return validPoints;
}

void drawLine(char bitMap[][IMAGE_SIZE], int x0, int y0, int x1, int y1) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int scaleX;
	int scaleY;
	int diff = dx - dy;

	//Finding scale for proper orientation
	if (x0 < x1)
		scaleX = 1;
	else
		scaleX = -1;
	if (y0 < y1)
		scaleY = 1;
	else
		scaleY = -1;

	while (true) {
		if (x0 >= 0 && x0 < IMAGE_SIZE && y0 >= 0 && y0 < IMAGE_SIZE) {
			bitMap[y0][x0] = 255; 
		}

		if (x0 == x1 && y0 == y1){
    	break;
    }

		int e2 = 2 * diff;
		if (e2 > -dy) {
			diff -= dy;
			x0 += scaleX;
		}

		if (e2 < dx) {
			diff += dx;
			y0 += scaleY;
		}
	}
}
