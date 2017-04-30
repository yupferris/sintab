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

static const int fastCosTabLog2Size = 10; // size = 1024
static const int fastCosTabSize = (1 << fastCosTabLog2Size);
static double fastCosTab[fastCosTabSize + 1];

void initFastCos()
{
	for (int i = 0; i < fastCosTabSize + 1; i++)
	{
		double phase = double(i) * ((M_PI * 2) / fastCosTabSize);
		fastCosTab[i] = cos(phase);
	}
}

double fastCos(double x)
{
	const int fastCosTabMask = fastCosTabSize - 1;

	const int fractBits = 16;
	const int fractScale = 1 << fractBits;
	const int fractMask = fractScale - 1;

	double phase = x * ((double)fastCosTabSize / (M_PI * 2) * (double)fractScale);
	unsigned long long phaseQuantized = (unsigned long long)(long long)phase;

	unsigned int whole = (unsigned int)phaseQuantized >> fractBits;
	unsigned int fract = (unsigned int)phaseQuantized & fractMask;

	int index = whole & fastCosTabMask;

	auto left = fastCosTab[index];
	auto right = fastCosTab[index + 1];

	double fractMix = (double)fract * (1.0 / fractScale);
	return left + (right - left) * fractMix;
}

double fastSin(double x)
{
	return fastCos(x - M_PI_2);
}

double func(double x)
{
	return cos(x);
}

double fastFunc(double x)
{
	return fastCos(x);
}

int main(int argc, char **argv)
{
	const int numIterations = 20000000;

	Timer timer;

	// Table init
	cout << "Table init: ";
	timer.Start();

	initFastCos();

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
	double maxError = 0.0;
	for (int exp = -10; exp < 10; ++exp)
	{
		for (double significand = 1.0; significand < 2.0; significand += 1e-5)
		{
			double x = ldexp(significand, exp);
			double orig = func(x);
			double fast = fastFunc(x);
			double error = abs(fast - orig);
			totalError += error;
			maxError = max(maxError, error);
		}
	}
	cout << totalError << endl;
	// Average error is total error / num iterations / range of func (in this case 1 - (-1) == 2)
	//  Not sure if this makes sense but it's some metric to optimize for at least :)
	double averageError = totalError / (double)numIterations / 2.0;
	cout << "Average error: " << (averageError * 100.0) << "%" << endl;
	cout << "Max error: " << maxError << endl;

	for (int exp = 0; exp < 52; ++exp)
	{
		for (double significand = 1.0; significand < 2.0; significand += 1e-1)
		{
			double x = ldexp(significand, exp);
			double orig = func(x);
			double fast = fastFunc(x);
			double error = abs(fast - orig);
			if (error > 0.1)
			{
				cout << "Range problems at exponent : " << exp << endl;
				goto done;
			}
		}
	}
done:

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