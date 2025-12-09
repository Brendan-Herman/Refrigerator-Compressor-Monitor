#ifndef TEMPANALYSIS_H
#define TEMPANALYSIS_H

#include <vector>

/**
 * Analyzes temperature data by calculating a z-score for the temperature trend.
 * 
 * This function computes the slope of the new temperature data and compares it
 * to a collection of historical slopes using z-score normalization.
 * 
 * @param slopes Vector of historical temperature slopes for comparison
 * @param newTemp Vector of new temperature readings (must have at least 10 elements)
 * @return Z-score indicating how the new temperature slope compares to historical slopes
 *         (positive = steeper increase than average, negative = less steep than average)
 */
float tempAnalysis(std::vector<float>& slopes, std::vector<float>& newTemp);

#endif