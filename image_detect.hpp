#pragma once

#include "json.hpp"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief  Convert image to base64 string
 * @note
 * @param  image: Image to convert
 * @retval Base64 string
 */
std::string img_to_base64(cv::Mat image);

/**
 * @brief  Convert base64 string to image
 * @note
 * @param  base64_str: string to convert
 * @retval Image
 */
cv::Mat base64_to_img(const std::string& base64_str);

/**
 * @brief  The image and the information needed for image recognition are sent to obtain the recognition result
 * @note
 * @param  detect_data: A json string of the image and the information needed for image recognition
 * @retval A json of Detection result
 */
nlohmann::json detect_image(nlohmann::json detect_data);
