#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <Arduino.h>
#include "vibration.h"

// FFT courtesy of https://www.w3computing.com/articles/how-to-implement-a-fast-fourier-transform-fft-in-cpp/

bool store_vibration()
{
    // store the vibration and detect if it is ready for transform
    if (data.size() == 2048)
    {
        return true;
    }
    data.push_back({(float)analogRead(PIEZO_PIN), 0.0f});
    return data.size() == 2048;
}

void erase()
{
    // reset the data vector
    while (data.size() > 0)
    {
        data.pop_back();
    }
}

bool detect_activity()
{
    // detect compressor activity
    bool active=false;
    for (int i = 0; i < 500; i++)
    {
        if (analogRead(PIEZO_PIN) > 250)
        {
            active=true;
            break;
        }
        delay(1);
    }
    return active;
}

void fft(CArray &x)
{
    // perform FFT in-place
    const size_t N = x.size();
    if (N <= 1)
        return;

    // Bit-reversed addressing permutation
    size_t j = 0;
    for (size_t i = 1; i < N; ++i)
    {
        size_t bit = N >> 1;
        while (j & bit)
        {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;

        if (i < j)
        {
            swap(x[i], x[j]);
        }
    }

    // Iterative FFT
    for (size_t len = 2; len <= N; len <<= 1)
    {
        double angle = -2 * pi / len;
        Complex wlen(cos(angle), sin(angle));
        for (size_t i = 0; i < N; i += len)
        {
            Complex w(1);
            for (size_t j = 0; j < len / 2; ++j)
            {
                Complex u = x[i + j];
                Complex v = x[i + j + len / 2] * w;
                x[i + j] = u + v;
                x[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

// convert complex vector to float vector
vector<float> magnitude(const CArray transform)
{
    // make the transform real
    vector<float> t;
    for (int i = 0; i < transform.size(); i++)
    {
        t.push_back(abs(transform[i]));
    }
    return t;
}

// compare two vectors
float compare(const vector<float> baseline, const vector<float> current)
{
    // Serial.println("Comparing vibrations");
    // Compare a baseline vector with a new vector
    if (baseline.size()!=current.size()){
        Serial.println("Cannot compare vectors of different sizes");
        return 0;
    }
    float diff = 0.0f;
    for (int i = 0; i < baseline.size(); i++)
    {
        diff += pow(baseline[i] - current[i], 2);
    }
    return diff;
}

vector<float> vectordifference(const vector<float> a, const vector<float> b){
    vector<float> temp;
    for(int i=0;i<2048;i++){
        temp.push_back(a[i]-b[i]);
    }
    return temp;
}

float vectorsize(const vector<float> a){
    float temp=0;
    for(int i=0;i<2048;i++){
        temp+=a[i]*a[i];
    }
    return pow(temp, 0.5f);
}

bool isOff(){
    for(int i=0;i<5000;i++){
        if(analogRead(PIEZO_PIN)>250){
            return false;
        }
        delay(1);
    }
    return true;
}

