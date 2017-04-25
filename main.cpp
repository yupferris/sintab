#include <iostream>
#include <string>
using namespace std;

#include <Windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

class Timer
{
public:
	void Start()
	{
		QueryPerformanceCounter(&startCounter);
	}

	void Stop()
	{
		LARGE_INTEGER endCounter, frequency;

		QueryPerformanceCounter(&endCounter);
		QueryPerformanceFrequency(&frequency);

		elapsedTimeUs = (endCounter.QuadPart - startCounter.QuadPart) * 1000000 / frequency.QuadPart;
	}

	long long ElapsedTimeUs()
	{
		return elapsedTimeUs;
	}

private:
	LARGE_INTEGER startCounter;
	long long elapsedTimeUs;
};

static const int fastSinTabSize = 1024; // Must be a power of 2

static double fastSinTab[fastSinTabSize];

void initFastSin()
{
	for (int i = 0; i < fastSinTabSize; i++)
	{
		double phase = (double)i / fastSinTabSize * M_PI;
		fastSinTab[i] = sin(phase);
	}
}

double fastSin(double x)
{
	const int fastSinTabMask = fastSinTabSize - 1;

	const int fractBits = 16;
	const int fractScale = 1 << fractBits;
	const int fractMask = fractScale - 1;

	double phase = x * ((double)fastSinTabSize / M_PI * (double)fractScale);
	unsigned long long phaseQuantized = (unsigned long long)(long long)phase;

	unsigned int whole = (unsigned int)phaseQuantized >> fractBits;
	unsigned int fract = (unsigned int)phaseQuantized & fractMask;

	int leftIndex = whole & fastSinTabMask;
	int rightIndex = (whole + 1) & fastSinTabMask;

	double left = fastSinTab[leftIndex];
	double right = fastSinTab[rightIndex];

	double fractMix = (double)fract * (1.0 / (double)fractScale);
	double result = left + (right - left) * fractMix;

	const int invertMask = fastSinTabSize;
	bool invert = (whole & invertMask) != 0;

	return invert ? -result : result;
}

double fastCos(double x)
{
	return fastSin(x + M_PI_2);
}

double func(double x)
{
	return sin(x + sin(x * 0.02f));
}

double fastFunc(double x)
{
	return fastSin(x + fastSin(x * 0.02f));
}

int main(int argc, char **argv)
{
	const int numIterations = 20000000;

	Timer timer;

	// Table init
	cout << "Table init: ";
	timer.Start();

	initFastSin();

	timer.Stop();
	cout << timer.ElapsedTimeUs() << "us" << endl;

	// Original timing
	cout << "Orig function: ";
	timer.Start();

	for (int i = 0; i < numIterations; i++)
	{
		volatile double res = func((double)(i - numIterations / 2));
	}

	timer.Stop();
	long long origTimeUs = timer.ElapsedTimeUs();
	cout << origTimeUs << "us" << endl;

	// Fast timing
	cout << "Fast function: ";
	timer.Start();

	for (int i = 0; i < numIterations; i++)
	{
		volatile double res = fastFunc((double)(i - numIterations / 2));
	}

	timer.Stop();
	long long fastTimeUs = timer.ElapsedTimeUs();
	cout << fastTimeUs << "us" << endl;

	// Improvement
	double improvement = (double)(origTimeUs - fastTimeUs) / (double)origTimeUs;
	cout << "Improvement: " << (improvement * 100.0) << "%" << endl;

	// Total error
	cout << "Total error: ";
	double totalError = 0.0;
	for (int i = 0; i < numIterations; i++)
	{
		double x = (double)i;
		double orig = func(x);
		double fast = fastFunc(x);
		totalError += abs(fast - orig);
	}
	cout << totalError << endl;
	// Average error is total error / num iterations / range of func (in this case 1 - (-1) == 2)
	//  Not sure if this makes sense but it's some metric to optimize for at least :)
	double averageError = totalError / (double)numIterations / 2.0;
	cout << "Average error: " << (averageError * 100.0) << "%" << endl;

	// Draw fn
	cout << "Graph:" << endl;
	const int gridWidth = 100;
	const int gridHeight = 20;
	const int gridSize = gridWidth * gridHeight;
	bool grid[gridSize];
	memset(grid, 0, gridSize * sizeof(bool));
	for (int i = 0; i < gridWidth; i++)
	{
		double phase = ((double)i / (double)gridWidth * 2.0 - 1.0) * (M_PI * 2.0);
		double res = fastSin(phase) * 0.5 + 0.5;
		int y = (int)(res * (double)(gridHeight - 1));
		grid[(gridHeight - 1 - y) * gridWidth + i] = true;
	}
	for (int y = 0; y < gridHeight; y++)
	{
		for (int x = 0; x < gridWidth; x++)
		{
			cout << string(grid[y * gridWidth + x] ? "*" : (x == gridWidth / 2 ? "|" : (y == gridHeight / 2 ? "-" : " ")));
		}
		cout << endl;
	}

	return 0;
}