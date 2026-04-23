#include "OCR.h"
#include "CTCCharBoxExtractor.h"
#include <QRegularExpression>
#include <QString>
#include <iostream>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

OCRPipeline::OCRPipeline() : env(ORT_LOGGING_LEVEL_WARNING, "OCR") {
    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    try {
        QString appDir = QCoreApplication::applicationDirPath();
        QString detPath = QDir(appDir).filePath("../assets/PP-OCRv5_server_det_infer.onnx");
        QString recPath = QDir(appDir).filePath("../assets/PP-OCRv5_server_rec_infer.onnx");

        detSession = std::make_unique<Ort::Session>(env, detPath.toStdString().c_str(), sessionOptions);
        recSession = std::make_unique<Ort::Session>(env, recPath.toStdString().c_str(), sessionOptions);
        
        QString dictPath = QDir(appDir).filePath("../assets/ppocrv5_dict.txt");
        
        QFile dictFile(dictPath);
        if (dictFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&dictFile);
            in.setEncoding(QStringConverter::Utf8);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                m_charDict.push_back(line.toStdString());
            }
            m_charDict.push_back(" "); // space character
            qDebug() << "Loaded OCR dictionary with" << m_charDict.size() << "entries.";
        } else {
            qDebug() << "Failed to load OCR dictionary:" << dictPath;
        }
        
    } catch(const Ort::Exception& e) {
        std::cerr << "Failed to load ONNX models: " << e.what() << std::endl;
    }
    m_charBoxExtractor = std::make_unique<CTCCharBoxExtractor>();
}

std::vector<OCRResult> OCRPipeline::processImage(const cv::Mat& image) {
    m_cancelled.store(false, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<OCRResult> results;
    if (!detSession || !recSession || image.empty()) {
        qDebug() << "OCR models failed to load. Returning mock debug text for UI selection testing.";
        results.push_back({"https://example.com", {cv::Point(50, 120), cv::Point(450, 120), cv::Point(450, 170), cv::Point(50, 170)}, {}, true});
        results.push_back({"OCR Model Failed", {cv::Point(50, 50), cv::Point(350, 50), cv::Point(350, 100), cv::Point(50, 100)}, {}, false});
        return results;
    }

    auto boxes = detectText(image);
    qDebug() << "OCR detectText found" << boxes.size() << "boxes";
    for (const auto& box : boxes) {
        if (m_cancelled.load(std::memory_order_relaxed)) {
            qDebug() << "OCR cancelled";
            return {};
        }
        auto [text, chars] = recognizeText(image, box);
        if (text.empty()) continue;
        bool is_url = checkIsUrl(text);
        qDebug() << "OCR recognized text:" << QString::fromStdString(text) << "is_url:" << is_url;
        results.push_back({text, box, chars, is_url});
    }
    return results;
}

std::vector<std::vector<cv::Point>> OCRPipeline::detectText(const cv::Mat& image) {
    std::vector<std::vector<cv::Point>> boxes;
    // Resize image to 640x640 (standard for detection)
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(640, 640));
    resized.convertTo(resized, CV_32FC3, 1.0f / 255.0f);
    
    // Normalize (mean, std)
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std(0.229, 0.224, 0.225);
    cv::subtract(resized, mean, resized);
    cv::divide(resized, std, resized);
    
    // HWC to CHW
    std::vector<cv::Mat> channels(3);
    cv::split(resized, channels);
    std::vector<float> input_tensor_values;
    for (int i = 0; i < 3; ++i) {
        input_tensor_values.insert(input_tensor_values.end(), (float*)channels[i].datastart, (float*)channels[i].dataend);
    }
    
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> input_shape = {1, 3, 640, 640};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());
    
    const char* input_names[] = {"x"};
    const char* output_names[] = {"fetch_name_0"};
    
    try {
        auto output_tensors = detSession->Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
        
        // Simple mock of DBNet post processing: find contours in thresholded output
        float* out_data = output_tensors[0].GetTensorMutableData<float>();
        cv::Mat out_mat(640, 640, CV_32FC1, out_data);
        
        double minV, maxV;
        cv::minMaxLoc(out_mat, &minV, &maxV);
        qDebug() << "Detection output min/max:" << minV << maxV;
        
        cv::Mat thresh;
        cv::threshold(out_mat, thresh, 0.3, 255, cv::THRESH_BINARY);
        thresh.convertTo(thresh, CV_8UC1);
        
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        float scale_x = (float)image.cols / 640.0f;
        float scale_y = (float)image.rows / 640.0f;
        
        for (const auto& contour : contours) {
            if (cv::contourArea(contour) < 100) continue;
            cv::RotatedRect rect = cv::minAreaRect(contour);
            float pad = 3.0f;
            rect = cv::RotatedRect(
                rect.center,
                cv::Size2f(rect.size.width + 2 * pad, rect.size.height + 2 * pad),
                rect.angle
            );
            cv::Point2f pts[4];
            rect.points(pts);
            std::vector<cv::Point> box;
            for (int i = 0; i < 4; ++i) {
                box.push_back(cv::Point(pts[i].x * scale_x, pts[i].y * scale_y));
            }
            boxes.push_back(box);
        }
    } catch (const std::exception& e) {
        qDebug() << "ONNX Detection Error:" << e.what();
    }
    
    return boxes;
}

RecognitionResult OCRPipeline::recognizeText(const cv::Mat& image, const std::vector<cv::Point>& box) {
    if (box.empty() || image.empty()) return {"", {}};
    
    // Crop text region
    cv::Rect bounding_rect = cv::boundingRect(box);
    bounding_rect &= cv::Rect(0, 0, image.cols, image.rows);
    if (bounding_rect.width <= 0 || bounding_rect.height <= 0) return {"", {}};
    
    cv::Mat cropped = image(bounding_rect);
    
    // Resize to 48x width (v4/v5 models use height 48 instead of 32)
    cv::Mat resized;
    int target_width = (int)(48.0f * cropped.cols / cropped.rows);
    target_width = std::max(target_width, 48);
    cv::resize(cropped, resized, cv::Size(target_width, 48));
    
    resized.convertTo(resized, CV_32FC3, 1.0f / 255.0f);
    cv::Scalar mean(0.5, 0.5, 0.5);
    cv::Scalar std(0.5, 0.5, 0.5);
    cv::subtract(resized, mean, resized);
    cv::divide(resized, std, resized);
    
    std::vector<cv::Mat> channels(3);
    cv::split(resized, channels);
    std::vector<float> input_tensor_values;
    for (int i = 0; i < 3; ++i) {
        input_tensor_values.insert(input_tensor_values.end(), (float*)channels[i].datastart, (float*)channels[i].dataend);
    }
    
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> input_shape = {1, 3, 48, target_width};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());
    
    const char* input_names[] = {"x"};
    const char* output_names[] = {"fetch_name_0"};
    
    try {
        auto output_tensors = recSession->Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
        
        // Simple CTC decode argmax
        const float* out_data = output_tensors[0].GetTensorData<float>();
        auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape(); // [1, seq_len, num_classes]
        int seq_len = shape[1];
        int num_classes = shape[2];
        
        std::string result = "";
        int last_idx = -1;
        for (int i = 0; i < seq_len; ++i) {
            int max_idx = 0;
            float max_val = -1e9;
            for (int j = 0; j < num_classes; ++j) {
                float val = out_data[i * num_classes + j];
                if (val > max_val) {
                    max_val = val;
                    max_idx = j;
                }
            }
            if (max_idx > 0 && max_idx != last_idx) { // Ignore blank and repeated
                int dict_idx = max_idx - 1; // 0 is blank
                if (dict_idx >= 0 && dict_idx < m_charDict.size()) {
                    result += m_charDict[dict_idx];
                }
            }
            last_idx = max_idx;
        }
        std::vector<CharBox> chars;
        if (m_charBoxExtractor) {
            chars = m_charBoxExtractor->extract(
                cropped, box, out_data, seq_len, num_classes, result, m_charDict
            );
        }
        return {result, chars};
    } catch (const std::exception& e) {
        qDebug() << "ONNX Recognition Error:" << e.what();
    }

    return {"", {}};
}

bool OCRPipeline::checkIsUrl(const std::string& text) {
    QRegularExpression url_regex(QStringLiteral(R"(^https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)$)"));
    QRegularExpressionMatch match = url_regex.match(QString::fromStdString(text));
    return match.hasMatch();
}
