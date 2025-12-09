#ifndef VIBRATION_H
#define VIBRATION_H

#include <vector>
#include <complex>
#include <cmath>

#define PIEZO_PIN 32
#define pi 3.141592653589793238462643383279502884197169399

using namespace std;
using Complex = complex<float>;
using CArray = vector<Complex>;

extern CArray data; //complex vector for storing vibration data temporarily
bool store_vibration();
bool detect_activity();
void fft(CArray& x);
vector<float> magnitude(const CArray transform);
float compare(const vector<float> baseline, const vector<float> current);
void erase();
bool isOff();
vector<float> vectordifference(const vector<float> a, const vector<float> b);
float vectorsize(const vector<float> a);

#endif
