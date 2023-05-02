// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include <bitset>

using namespace std;

Scalar median(Mat_<uchar> window) {
    Mat_<uchar> sorted;
    cv::sort(window, sorted, SORT_ASCENDING);
    int middle = window.total() / 2;
    if (window.total() % 2 == 0) {
        return (sorted(middle) + sorted(middle - 1)) / 2;
    }
    else {
        return sorted(middle);
    }
}


Mat_<uchar> createCensusTransform(Mat_<uchar> image, int window_size)
{
    int half_window_size = window_size / 2;

    Mat_<uchar> resized_image;
    resize(image, resized_image, Size(image.cols + 2 * half_window_size, image.rows + 2 * half_window_size), 0, 0, INTER_LINEAR);

    Mat_<uchar> padded_image;
    copyMakeBorder(resized_image, padded_image, half_window_size, half_window_size, half_window_size, half_window_size, BORDER_REFLECT);


    Mat_<uchar> census = Mat::zeros(image.size(), CV_32SC1);

    for (int row = 0; row < image.rows; row++)
    {
        for (int col = 0; col < image.cols; col++)
        {
            Mat_<uchar> window = padded_image(Rect(col, row, window_size, window_size));

            vector<bool> binary_string;
            for (int i = 0; i < window_size; i++)
            {
                for (int j = 0; j < window_size; j++) {
                    if (i != (window_size / 2) || j != (window_size / 2))
                    {
                        binary_string.push_back(window(window_size/2, window_size/2) >= window(i,j));
                    }
                }
            }
            uchar uchar_val = 0;

            // Convert binary vector to unsigned char
            for (size_t i = 0; i < binary_string.size(); ++i) {
                uchar_val |= binary_string[i] << i;
            }
            census(row, col) = uchar_val;
        }
    }

    return census;
}

int hammingDistance(string left_block, string right_block)
{
    int dist = 0;
    for (size_t i = 0; i < min(left_block.size(), right_block.size()); i++)
    {
        dist += ((int)left_block[i] ^ (int)right_block[i]);
    }
    return dist;
}

Mat_<uchar> computeDisparityMap(Mat_<uchar> left_census, Mat_<uchar> right_census, int block_size, int max_disp)
{
    int half_block_size = block_size / 2;

    Mat_<uchar> padded_left_census, padded_right_census;
    copyMakeBorder(left_census, padded_left_census, half_block_size, half_block_size, half_block_size, half_block_size, BORDER_REFLECT);
    copyMakeBorder(right_census, padded_right_census, half_block_size, half_block_size, half_block_size, half_block_size, BORDER_REFLECT);

    Mat_<uchar> disparity_map = Mat::zeros(left_census.size(), CV_8UC1);

    for (int row = 0; row < left_census.rows; row++)
    {
        for (int col = 0; col < left_census.cols; col++)
        {
            Mat_<uchar> left_block = padded_left_census(Rect(col, row, block_size, block_size));

            string left_block_string = "";
            for (int i = 0; i < block_size; i++)
            {
                for (int j = 0; j < block_size; j++)
                {
                    left_block_string += to_string((int)(left_block(i, j)));
                }
            }

            int min_dist = INT_MAX;
            int min_disp = 0;

            for (int disp = 0; disp < max_disp; disp++) {
                if (col - disp >= 0)
                {
                    Mat_<uchar> right_block = padded_right_census(Rect(col - disp, row, block_size, block_size));

                    string right_block_string = "";
                    for (int i = 0; i < right_block.rows; i++)
                    {
                        for (int j = 0; j < right_block.cols; j++)
                        {
                            right_block_string += to_string((int)(right_block(i, j)));
                        }
                    }

                    int dist = hammingDistance(left_block_string, right_block_string);

                    if (dist < min_dist)
                    {
                        min_dist = dist;
                        min_disp = disp;
                    }
                }
            }

            disparity_map(row, col) = min_disp;
        }
    }
    return disparity_map;
}


int main()
{
    Mat_<uchar> img_left = imread("Images\\left.png", IMREAD_GRAYSCALE);
    Mat_<uchar> img_right = imread("Images\\right.png", IMREAD_GRAYSCALE);    
    
    //imshow("img_left", img_left);
    //imshow("img_right", img_right);
    //waitKey(0);

    // Apply Census
    int window_size = 3;
    Mat_<uchar> left_census;
    Mat_<uchar> right_census;

    try {
        left_census = imread("Images\\Census_left.png", IMREAD_GRAYSCALE);
        right_census = imread("Images\\Census_right.png", IMREAD_GRAYSCALE);
    }
    catch (Exception e) {
        left_census = createCensusTransform(img_left, window_size);
        right_census = createCensusTransform(img_right, window_size);
    }


    //imshow("left_census", left_census);
    //imshow("right_census", right_census);
    //waitKey(0);

    // Compute the disparity map based on the Hamming distance
    Mat_<uchar> disparity_map = computeDisparityMap(left_census, right_census, 3, 128);
 
    imshow("disparity_map", disparity_map);
    waitKey(0);
	return 0;
}