#include <unistd.h>

#include <climits>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace dnn;
using namespace std;

const float confThreshold = 0.5, nmsThreshold = 0.4;
static std::vector<std::string> classes;

std::string getCameraDemoBase();

static const std::string camera_demo_base = getCameraDemoBase();
static const std::string model_prototxt = camera_demo_base + "/" + "yolov3.cfg";
static const std::string model_binary =
    camera_demo_base + "/" + "yolov3.weights";
static const std::string class_names =
    camera_demo_base + "/" + "object_detection_classes_yolov3.txt";

const float scale = 0.00392;
const Scalar mean = {0,0,0};
const bool swapRB = true;
const int inpWidth = 416;
const int inpHeight = 416;

static cv::dnn::Net net = cv::dnn::readNet(model_binary, model_prototxt);
static std::vector<String> outNames = net.getUnconnectedOutLayersNames();

std::string getCameraDemoBase()
{
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1) {
        throw std::runtime_error("Cannot readlink /proc/self/exe.");
    }
    buf[len] = '\0';
    std::string camera_demo_executable = std::string(buf);
    return camera_demo_executable.substr(
        0, camera_demo_executable.rfind(std::string("/")));
}

void loadClassNames()
{
    std::ifstream ifs(class_names);
    if (!ifs.is_open())
        cerr << "File " << class_names << " not found" <<endl;
    std::string line;
    while (std::getline(ifs, line))
    {
        classes.push_back(line);
    }
}

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

    std::string label = format("%.2f", conf);
    
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - labelSize.height),
        Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
}

inline void preprocess(const Mat& frame, Net& net, Size inpSize, float scale,
    const Scalar& mean, bool swapRB)
{
    
    static Mat blob;
    // Create a 4D blob from a frame.
    if (inpSize.width <= 0) inpSize.width = frame.cols;
    if (inpSize.height <= 0) inpSize.height = frame.rows;
    blobFromImage(frame, blob, 1.0, inpSize, Scalar(), swapRB, false, CV_8U);

    // Run a model.
    net.setInput(blob, "", scale, mean);
    if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
    {
        resize(frame, frame, inpSize);
        Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
        net.setInput(imInfo, "im_info");
    }
}

std::vector<std::string> postprocess(Mat& frame, const std::vector<Mat>& outs, Net& net)
{
    std::vector<std::string> detected_object_classes;
    static std::vector<int> outLayers = net.getUnconnectedOutLayers();
    static std::string outLayerType = net.getLayer(outLayers[0])->type;

    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect> boxes;
    if (outLayerType == "DetectionOutput")
    {
        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        CV_Assert(outs.size() > 0);
        for (size_t k = 0; k < outs.size(); k++)
        {
            float* data = (float*)outs[k].data;
            for (size_t i = 0; i < outs[k].total(); i += 7)
            {
                float confidence = data[i + 2];
                if (confidence > confThreshold)
                {
                    int left   = (int)data[i + 3];
                    int top    = (int)data[i + 4];
                    int right  = (int)data[i + 5];
                    int bottom = (int)data[i + 6];
                    int width  = right - left + 1;
                    int height = bottom - top + 1;
                    if (width <= 2 || height <= 2)
                    {
                        left   = (int)(data[i + 3] * frame.cols);
                        top    = (int)(data[i + 4] * frame.rows);
                        right  = (int)(data[i + 5] * frame.cols);
                        bottom = (int)(data[i + 6] * frame.rows);
                        width  = right - left + 1;
                        height = bottom - top + 1;
                    }
                    classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                    boxes.push_back(Rect(left, top, width, height));
                    confidences.push_back(confidence);
                }
            }
        }
    }
    else if (outLayerType == "Region")
    {
        for (size_t i = 0; i < outs.size(); ++i)
        {
            // Network produces output blob with a shape NxC where N is a number of
            // detected objects and C is a number of classes + 4 where the first 4
            // numbers are [center_x, center_y, width, height]
            float* data = (float*)outs[i].data;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
            {
                Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                Point classIdPoint;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                if (confidence > confThreshold)
                {
                    int centerX = (int)(data[0] * frame.cols);
                    int centerY = (int)(data[1] * frame.rows);
                    int width = (int)(data[2] * frame.cols);
                    int height = (int)(data[3] * frame.rows);
                    int left = centerX - width / 2;
                    int top = centerY - height / 2;

                    classIds.push_back(classIdPoint.x);
                    confidences.push_back((float)confidence);
                    boxes.push_back(Rect(left, top, width, height));
                }
            }
        }
    }
    else
        cerr << "Unknown output layer type: "  << outLayerType << endl;;

    std::vector<int> indices;
    NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        detected_object_classes.push_back(classes[classIds[idx]]);
        Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }

    return detected_object_classes;
}

std::vector<std::string> processImage(const char *fileName)
{
    if (classes.empty())
    {
        loadClassNames();
    }

    cv::Mat img = cv::imread(fileName);
    
    preprocess(img, net, Size(inpWidth, inpHeight), scale, ::mean, swapRB);

    std::vector<Mat> outs;
    net.forward(outs, outNames);

    std::vector<std::string> detectedObjects = postprocess(img, outs, net);

    /*// Put efficiency information.
    std::vector<double> layersTimes;
    double freq = getTickFrequency() / 1000;
    double t = net.getPerfProfile(layersTimes) / freq;
    std::string label = format("Inference time: %.2f ms", t);
    putText(img, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));*/

    imshow("GaiaDemo", img);
    return detectedObjects;
}