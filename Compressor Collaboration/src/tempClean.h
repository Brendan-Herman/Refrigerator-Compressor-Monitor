#ifndef TEMPCLEAN_H
#define TEMPCLEAN_H

#include <vector>

/*
 * Processes baseline temperature data to calculate temperature slopes.
 * 
 * For each vector in the baseline data, this function calculates the slope
 * between the mean of the first 5 temperatures and the mean of the last 5
 * temperatures.
 * 
 * @param tempBaseline 2D vector containing multiple temperature measurement series
 *                     (each inner vector must have at least 10 elements)
 * @return Vector of slopes, one for each temperature series in tempBaseline
 */
 
float tempClean(std::vector<float>& temperatures);

#endif