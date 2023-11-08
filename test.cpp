#include "image_detect.hpp"
#include "json.hpp"
#include <curl/curl.h>
#include <fmt/core.h>
using fmt::print;
using fmt::println;

int main() {
    std::string json_str = R"(
        {
            "Base64Img": "",
            "Alg": [
                {
                    "Cls": [4, 5],
                    "Range": 3,
                    "Conf": 0.8,
                    "Rect": [63,165,219,339],
                    "Name": "指针仪表",
                    "DeviceCode": "123456",
                    "ModelCode": "KeypointMeter"
                }
            ]
        }
    )";
    cv::Mat image = cv::imread("test.jpg");
    auto base64 = img_to_base64(image);
    auto json_config = nlohmann::json::parse(json_str);
    json_config["Base64Img"] = base64;
    auto result = detect_image(json_config);
    fmt::println("{}", result.dump(4));
    cv::imwrite("output.jpg", base64_to_img(result["Base64Img"]));
}