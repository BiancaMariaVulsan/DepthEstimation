// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include <bitset>

using namespace std;

// cu fereastra mai mare si mat<int> pt dim 5
// compar disparitatea cu cati pixeli la dif de < 2
// incerc pe alta imagine mai mica sau o redimensionez pe asta

Mat_<int> createCensusTransform(Mat_<uchar> image, int window_size)
{
    cout << "Computing Census Transform" << endl;
    int half_window_size = window_size / 2;

    Mat_<uchar> resized_image;
    resize(image, resized_image, Size(image.cols + 2 * half_window_size, image.rows + 2 * half_window_size), 0, 0, INTER_LINEAR);

    Mat_<uchar> padded_image;
    copyMakeBorder(resized_image, padded_image, half_window_size, half_window_size, half_window_size, half_window_size, BORDER_REFLECT);


    Mat_<int> census(image.rows, image.cols);
    census.setTo(0);

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
                        binary_string.push_back(window(window_size / 2, window_size / 2) >= window(i, j));
                    }
                }
            }
            int int_val = 0;

            // Convert binary vector to int
            for (size_t i = 0; i < binary_string.size(); ++i) {
                int_val |= binary_string[i] << i;
            }
            census(row, col) = int_val;
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

void medianFilter(Mat_<uchar> src, int w)
{
    Mat_<uchar> dst = src.clone();

    double t = (double)getTickCount();
    for (int i = w / 2; i < src.rows - w / 2; i++)
    {
        for (int j = w / 2; j < src.cols - w / 2; j++)
        {
            std::vector<uchar> v;

            for (int k = i - w / 2; k <= i + w / 2; k++)
            {
                for (int l = j - w / 2; l <= j + w / 2; l++)
                {
                    v.push_back(src(k, l));
                }
            }

            std::sort(v.begin(), v.end());
            dst.at<uchar>(i, j) = v[v.size() / 2];
        }
    }

    imshow("disparity", dst);
    waitKey(0);
    imwrite("Images\\Results\\disparity_map_view_5_5_filtered.bmp", dst);
}

Mat_<uchar> computeDisparityMap(Mat_<int> left_census, Mat_<int> right_census, int block_size, int max_disp)
{
    cout << "Computing disparity map" << endl;
    int half_block_size = block_size / 2;

    Mat_<int> padded_left_census, padded_right_census;
    copyMakeBorder(left_census, padded_left_census, half_block_size, half_block_size, half_block_size, half_block_size, BORDER_REFLECT);
    copyMakeBorder(right_census, padded_right_census, half_block_size, half_block_size, half_block_size, half_block_size, BORDER_REFLECT);

    Mat_<uchar> disparity_map = Mat::zeros(left_census.size(), CV_8UC1);

    for (int row = 0; row < left_census.rows; row++)
    {
        for (int col = 0; col < left_census.cols; col++)
        {
            Mat_<int> left_block = padded_left_census(Rect(col, row, block_size, block_size));

            string left_block_string = "";
            for (int i = 0; i < block_size; i++)
            {
                for (int j = 0; j < block_size; j++)
                {
                    left_block_string += bitset<32>(left_block(i, j)).to_string();
                }
            }

            int min_dist = INT_MAX;
            int min_disp = 0;

            for (int disp = 0; disp < max_disp; disp++) {
                if (col - disp >= 0)
                {
                    Mat_<int> right_block = padded_right_census(Rect(col - disp, row, block_size, block_size));

                    string right_block_string = "";
                    for (int i = 0; i < right_block.rows; i++)
                    {
                        for (int j = 0; j < right_block.cols; j++)
                        {
                            right_block_string += bitset<32>(right_block(i, j)).to_string();
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

            disparity_map(row, col) = min_disp * 16;
        }
    }
    return disparity_map;
}


Mat_<uchar> computeDisparity(Mat_<uchar> left, Mat_<uchar> right, int block_size, int max_disp)
{
	Mat_<int> left_census = createCensusTransform(left, block_size);
	Mat_<int> right_census = createCensusTransform(right, block_size);

	Mat_<uchar> disparity_map = computeDisparityMap(left_census, right_census, block_size, max_disp);

	return disparity_map;
}


int compareImages(const Mat_<uchar>& myImg, const Mat_<uchar>& original)
{
    int count = 0;

    for (int i = 0; i < min(myImg.rows, original.rows); i++)
    {
        for (int j = 0; j < min(myImg.cols, original.cols); j++)
        {
            if (abs(myImg(i, j) - original(i, j)) <= 2)
            {
                count++;
            }
        }
    }

    return count * 100 / (myImg.rows * myImg.cols); // percentage
}


int main()
{
    Mat_<uchar> img_left = imread("Images\\l.jpeg", IMREAD_GRAYSCALE);
    Mat_<uchar> img_right = imread("Images\\r.jpeg", IMREAD_GRAYSCALE);
    Mat_<uchar> result = imread("Images\\output.jpeg", IMREAD_GRAYSCALE);

    // Apply Census
    int window_size = 3;
    Mat_<int> left_census;
    Mat_<int> right_census;
    left_census = createCensusTransform(img_left, window_size);
    right_census = createCensusTransform(img_right, window_size);

    // Compute the disparity map based on the Hamming distance
    Mat_<uchar> disparity_map = computeDisparityMap(left_census, right_census, 3, 16);

    // Apply median filter to reduce the salt and papepr noise
    medianFilter(disparity_map, 5);

    cout << "Similarity percent: " << compareImages(disparity_map, result);
    return 0;
}