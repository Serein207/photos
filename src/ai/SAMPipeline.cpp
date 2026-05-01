#include "SAMPipeline.h"
#include "AIEngine.h"
#include <iostream>
#include <vector>

// Decoder tensor names (samexporter format).
static const char* DECODER_IN_IMAGE_EMBEDDINGS = "image_embeddings";
static const char* DECODER_IN_POINT_COORDS      = "point_coords";
static const char* DECODER_IN_POINT_LABELS      = "point_labels";
static const char* DECODER_IN_MASK_INPUT        = "mask_input";
static const char* DECODER_IN_HAS_MASK_INPUT    = "has_mask_input";
static const char* DECODER_IN_ORIG_IM_SIZE      = "orig_im_size";

static constexpr int PROMPT_COUNT = 1;
static constexpr int DECODER_MASK_SIZE = 256;

SAMPipeline::SAMPipeline(const std::string& encoderPath,
                         const std::string& decoderPath) {
    try {
        encoderEngine_ = std::make_unique<AIEngine>(encoderPath);
        decoderEngine_ = std::make_unique<AIEngine>(decoderPath);
    } catch (const std::exception& e) {
        std::cerr << "SAMPipeline: failed to load models — " << e.what() << std::endl;
        encoderEngine_.reset();
        decoderEngine_.reset();
    }
}

SAMPipeline::~SAMPipeline() = default;

bool SAMPipeline::isModelLoaded() const {
    return encoderEngine_ && encoderEngine_->getSession()
        && decoderEngine_ && decoderEngine_->getSession();
}

cv::Mat SAMPipeline::preprocessImage(const cv::Mat& bgrImage,
                                      cv::Size& resizedSize,
                                      cv::Mat& transformMatrix) {
    int origW = bgrImage.cols;
    int origH = bgrImage.rows;

    double scale = static_cast<double>(TARGET_SIZE) / std::max(origW, origH);
    int newW = static_cast<int>(origW * scale + 0.5);
    int newH = static_cast<int>(origH * scale + 0.5);

    resizedSize = cv::Size(newW, newH);

    transformMatrix = (cv::Mat_<double>(2, 3)
        << scale, 0, 0,
           0, scale, 0);

    cv::Mat warped;
    cv::warpAffine(bgrImage, warped, transformMatrix,
                   cv::Size(ENCODER_WIDTH, ENCODER_HEIGHT),
                   cv::INTER_LINEAR, cv::BORDER_CONSTANT,
                   cv::Scalar(128, 128, 128));

    cv::Mat floatImage;
    warped.convertTo(floatImage, CV_32FC3);
    return floatImage;
}

Ort::Value SAMPipeline::runEncoder(const cv::Mat& preprocessedImage) {
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> shape = {ENCODER_HEIGHT, ENCODER_WIDTH, 3};

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        reinterpret_cast<float*>(preprocessedImage.data),
        preprocessedImage.total() * preprocessedImage.channels(),
        shape.data(), shape.size());

    auto* session = encoderEngine_->getSession();
    Ort::AllocatorWithDefaultOptions allocator;
    auto inputName = session->GetInputNameAllocated(0, allocator);
    auto outputName = session->GetOutputNameAllocated(0, allocator);

    const char* inputNames[] = {inputName.get()};
    const char* outputNames[] = {outputName.get()};
    auto outputTensors = session->Run(Ort::RunOptions{nullptr},
                                       inputNames, &inputTensor, 1,
                                       outputNames, 1);

    return std::move(outputTensors[0]);
}

cv::Mat SAMPipeline::runDecoder(Ort::Value& imageEmbeddings,
                                 const cv::Size& originalSize,
                                 const cv::Mat& transformMatrix) {
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    float normPoints[PROMPT_COUNT][2] = {
        {0.50f, 0.50f}
    };

    // Convert normalized coords to original pixel coords, then apply
    // the same transform matrix as the image to get decoder-space coords.
    std::vector<float> coords((PROMPT_COUNT + 1) * 2);
    std::vector<float> labels(PROMPT_COUNT + 1);

    for (int i = 0; i < PROMPT_COUNT; ++i) {
        double px = normPoints[i][0] * originalSize.width;
        double py = normPoints[i][1] * originalSize.height;
        double tx = transformMatrix.at<double>(0, 0) * px
                  + transformMatrix.at<double>(0, 1) * py
                  + transformMatrix.at<double>(0, 2);
        double ty = transformMatrix.at<double>(1, 0) * px
                  + transformMatrix.at<double>(1, 1) * py
                  + transformMatrix.at<double>(1, 2);
        coords[i * 2]     = static_cast<float>(tx);
        coords[i * 2 + 1] = static_cast<float>(ty);
        labels[i] = 1.0f;
    }

    // Padding point (required by SAM decoder).
    coords[PROMPT_COUNT * 2]     = 0.0f;
    coords[PROMPT_COUNT * 2 + 1] = 0.0f;
    labels[PROMPT_COUNT] = -1.0f;

    std::vector<int64_t> coordsShape  = {1, PROMPT_COUNT + 1, 2};
    std::vector<int64_t> labelsShape  = {1, PROMPT_COUNT + 1};
    std::vector<int64_t> maskShape    = {1, 1, DECODER_MASK_SIZE, DECODER_MASK_SIZE};
    std::vector<int64_t> hasMaskShape = {1};
    std::vector<int64_t> sizeShape    = {2};

    std::vector<float> zeroMask(DECODER_MASK_SIZE * DECODER_MASK_SIZE, 0.0f);
    std::vector<float> noMask   = {0.0f};
    std::vector<float> origSize = {
        static_cast<float>(ENCODER_HEIGHT),
        static_cast<float>(ENCODER_WIDTH)
    };

    std::vector<Ort::Value> inputTensors;
    inputTensors.push_back(std::move(imageEmbeddings));
    inputTensors.push_back(Ort::Value::CreateTensor<float>(
        memoryInfo, coords.data(), coords.size(),
        coordsShape.data(), coordsShape.size()));
    inputTensors.push_back(Ort::Value::CreateTensor<float>(
        memoryInfo, labels.data(), labels.size(),
        labelsShape.data(), labelsShape.size()));
    inputTensors.push_back(Ort::Value::CreateTensor<float>(
        memoryInfo, zeroMask.data(), zeroMask.size(),
        maskShape.data(), maskShape.size()));
    inputTensors.push_back(Ort::Value::CreateTensor<float>(
        memoryInfo, noMask.data(), noMask.size(),
        hasMaskShape.data(), hasMaskShape.size()));
    inputTensors.push_back(Ort::Value::CreateTensor<float>(
        memoryInfo, origSize.data(), origSize.size(),
        sizeShape.data(), sizeShape.size()));

    const char* inputNames[] = {
        DECODER_IN_IMAGE_EMBEDDINGS,
        DECODER_IN_POINT_COORDS,
        DECODER_IN_POINT_LABELS,
        DECODER_IN_MASK_INPUT,
        DECODER_IN_HAS_MASK_INPUT,
        DECODER_IN_ORIG_IM_SIZE,
    };

    Ort::AllocatorWithDefaultOptions allocator;
    auto maskOutputName = decoderEngine_->getSession()->GetOutputNameAllocated(
        0, allocator);
    const char* outputNames[] = {maskOutputName.get()};

    auto outputTensors = decoderEngine_->getSession()->Run(
        Ort::RunOptions{nullptr},
        inputNames, inputTensors.data(), inputTensors.size(),
        outputNames, 1);
    // outputTensors[0] = masks: [1, C, H, W]. Pick best mask.
    float* maskData = outputTensors[0].GetTensorMutableData<float>();
    auto maskShapeOut = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    int maskC = static_cast<int>(maskShapeOut[1]);
    int maskH = static_cast<int>(maskShapeOut[2]);
    int maskW = static_cast<int>(maskShapeOut[3]);

    int bestIdx = 0;
    double bestScore = -1.0;
    for (int c = 0; c < maskC; ++c) {
        cv::Mat raw(maskH, maskW, CV_32FC1, maskData + c * maskH * maskW);
        cv::Mat clamped;
        cv::min(cv::max(raw, 0.0f), 1.0f, clamped);
        double m = cv::mean(clamped)[0];
        double score = 1.0 - std::abs(m - 0.5) * 2.0;
        if (score > bestScore) { bestScore = score; bestIdx = c; }
    }

    cv::Mat bestRaw(maskH, maskW, CV_32FC1, maskData + bestIdx * maskH * maskW);
    cv::Mat clamped;
    cv::min(cv::max(bestRaw, 0.0f), 1.0f, clamped);

    float centerVal = clamped.at<float>(maskH / 2, maskW / 2);
    if (centerVal < 0.5f) {
        clamped = 1.0f - clamped;
    }

    return clamped.clone();
}

cv::Mat SAMPipeline::postprocessMask(const cv::Mat& mask,
                                      const cv::Size& originalSize,
                                      const cv::Mat& transformMatrix) {
    cv::Mat invTransform;
    cv::invertAffineTransform(transformMatrix, invTransform);

    cv::Mat warped;
    cv::warpAffine(mask, warped, invTransform, originalSize,
                   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));

    cv::Mat blurred;
    cv::GaussianBlur(warped, blurred, cv::Size(5, 5), 0);
    return blurred;
}

cv::Mat SAMPipeline::inferMask(const cv::Mat& bgrImage) {
    if (!isModelLoaded() || bgrImage.empty()) {
        return cv::Mat();
    }

    std::lock_guard<std::mutex> lock(inferMutex_);

    try {
        cv::Size resizedSize;
        cv::Mat transformMatrix;
        cv::Mat preprocessed = preprocessImage(bgrImage, resizedSize, transformMatrix);

        Ort::Value embeddings = runEncoder(preprocessed);

        cv::Mat mask = runDecoder(embeddings, bgrImage.size(),
                                  transformMatrix);

        return postprocessMask(mask, bgrImage.size(), transformMatrix);
    } catch (const std::exception& e) {
        std::cerr << "SAMPipeline::inferMask error: " << e.what() << std::endl;
        return cv::Mat();
    }
}
