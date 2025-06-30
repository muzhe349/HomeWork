#include<iostream>
#include<windows.h>
#include<opencv2/opencv.hpp>
using namespace std;
using namespace cv;

// 全局变量用于存储处理结果
Mat originalImage, processedImage, resultImage;
vector<Mat> characters;
vector<string> recognitionResults;
bool showOriginal = true;
bool showProcessed = false;
bool showResult = false;

// 鼠标回调函数
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        // 点击切换显示模式
        showOriginal = !showOriginal;
        showProcessed = !showProcessed;
        
        if (showOriginal) {
            imshow("车牌识别系统", originalImage);
        } else if (showProcessed) {
            imshow("车牌识别系统", processedImage);
        }
    }
}

// 创建控制面板
void createControlPanel() {
    Mat controlPanel = Mat::zeros(100, 400, CV_8UC3);
    
    // 绘制按钮
    rectangle(controlPanel, Point(10, 10), Point(90, 40), Scalar(0, 255, 0), -1);
    putText(controlPanel, "原图", Point(25, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 0), 2);
    
    rectangle(controlPanel, Point(110, 10), Point(190, 40), Scalar(255, 0, 0), -1);
    putText(controlPanel, "处理", Point(125, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    
    rectangle(controlPanel, Point(210, 10), Point(290, 40), Scalar(0, 0, 255), -1);
    putText(controlPanel, "结果", Point(225, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    
    // 显示识别结果
    string resultText = "识别结果: ";
    for (const auto& result : recognitionResults) {
        resultText += result + " ";
    }
    putText(controlPanel, resultText, Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
    
    imshow("控制面板", controlPanel);
}

void showimg(Mat img) {
    if (img.data == nullptr) {//nullptr是空指针常量
        cout << "img.data is null" << endl;
    }
    else {
        imshow("img", img);//窗口名，图片
        waitKey(0);//按下任意键退出
    }
    cout << "图像宽为：" << img.cols ;//列
    cout<< "\t高度为：" << img.rows ;//行
    cout<< "\t通道数为：" << img.channels() << endl;//图像每个像素包含的数据维度
}

bool CompareRectArea(const Rect& a, const Rect& b) {
        return a.area() > b.area();
}

bool CompareRectX(const cv::Rect& a, const cv::Rect& b) {
    return a.x < b.x; // 按x坐标从小到大排序
}

Mat resizeToMinWidth(const Mat& src, int minWidth) {
    float scale = float(minWidth) / src.cols;      // 计算缩放比例
    int newHeight = int(src.rows * scale);         // 新高度
    Mat dst;
    resize(src, dst, Size(minWidth, newHeight));   // 缩放到指定宽度
    return dst;
}

//字符图片处理函数
Mat charImgProcess(const Mat& charImg) {
    int imgSize=32;   //imgSize 为目标边长
    // 1. 等比缩放
    int h = charImg.rows, w = charImg.cols;
    float scale = float(imgSize) / max(h, w);
    int newW = int(w * scale);
    int newH = int(h * scale);
    Mat resizedChar;
    resize(charImg, resizedChar, Size(newW, newH));

    // 2. 填充为正方形
    int top = (imgSize - newH) / 2;
    int bottom = imgSize - newH - top;
    int left = (imgSize - newW) / 2;
    int right = imgSize - newW - left;
    Mat paddedChar;//填充后的
                                                                               //填充函数，边框类型：常数     边框黑色
    copyMakeBorder(resizedChar, paddedChar, top, bottom, left, right, cv::BORDER_CONSTANT, 0);

    // 3. 灰度拉伸（线性归一化到0~255）
    double minVal, maxVal;
    minMaxLoc(paddedChar, &minVal, &maxVal);
    Mat stretched;//拉伸过的图像
    if (maxVal - minVal > 1e-3) {
        //灰度拉伸函数
        paddedChar.convertTo(stretched, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    } else {
        stretched = paddedChar.clone();
    }

    // 4. Otsu二值化
    Mat binaryChar;
    threshold(stretched, binaryChar, 0, 255, THRESH_BINARY |THRESH_OTSU);//大津法拉伸

    // 5. 开运算去噪
    Mat cleaned;
    morphologyEx(binaryChar, cleaned, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(2, 2)));


    //6.膨胀(改进)
    Mat morphed;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));//生成一个矩形结构元素
    dilate(cleaned, morphed, kernel);//使用矩形结构进行膨胀处理
    // showimg(morphed);

    return morphed;
}



    //字符匹配函数
    string recognizeChar(const Mat& charImg, vector<Mat> templates, vector<string> labels) {
        Mat processed = charImgProcess(charImg).clone();  // 先处理输入图像
        double bestMatch = -1;
        int bestIndex = -1;

        for (int i = 0; i < templates.size(); i++) {
            Mat result;
            Mat t=charImgProcess(templates[i]).clone();
            matchTemplate(processed,t, result, TM_CCOEFF_NORMED);
            double match = result.at<float>(0, 0);
            if (match > bestMatch) {
                bestMatch = match;
                bestIndex = i;
            }
        }
    cout<<bestMatch<<endl;
        if (bestMatch > 0.30) {  // 阈值可调
            return labels[bestIndex];
        }
        else return "?";
    }

// 字符匹配函数（使用TM_SQDIFF_NORMED，分数越小越好）
// string recognizeChar(const Mat& charImg, vector<Mat> templates, vector<string> labels) {
//     Mat processed = charImgProcess(charImg);  // 先处理输入图像
//
//     double bestMatch = 1e9; // 初始值设为很大
//     int bestIndex = -1;
//
//     for (int i = 0; i < templates.size(); i++) {
//         Mat result;
//         matchTemplate(processed, charImgProcess(templates[i]), result, TM_SQDIFF_NORMED);
//         double match = result.at<float>(0, 0);
//         // 分数越小越好
//         if (match < bestMatch) {
//             bestMatch = match;
//             bestIndex = i;
//         }
//     }
//     cout<<bestMatch<<endl;
//     // 阈值可调，通常0.3~0.4
//     if (bestMatch < 0.3) {
//         return labels[bestIndex];
//     } else {
//         return "?";
//     }
// }

Mat removeBottomWhiteBlock(const Mat& charImg, double white_ratio = 0.8, int min_block_height = 3) {
    CV_Assert(charImg.type() == CV_8U);
    int h = charImg.rows, w = charImg.cols;
    int last_row = h;
    int white_block_count = 0;
    // 从下往上，统计连续白色行
    for (int row = h - 1; row >= 0; row--) {
        int white_count = 0;
        for (int col = 0; col < w; col++) {
            if (charImg.at<uchar>(row, col) == 255) white_count++;
        }
        if (white_count > w * white_ratio) {
            white_block_count++;
            last_row = row;
        } else {
            // 如果已经有一片白色块，且高度达到要求，则裁剪
            if (white_block_count >= min_block_height) {
                break;
            } else {
                // 否则重置
                white_block_count = 0;
                last_row = h;
            }
        }
    }
    if (last_row < h) {
        return charImg.rowRange(0, last_row);
    } else {
        return charImg.clone();
    }
}

int main()    {
    SetConsoleOutputCP(65001);
    Mat image;//mat定义矩阵变量
    image = imread("D:\\code\\vscode_opencv\\333.jpg");//读取图片
    
    // 保存原图用于显示
    originalImage = image.clone();
    
    // 创建窗口并设置鼠标回调
    namedWindow("车牌识别系统", WINDOW_AUTOSIZE);
    setMouseCallback("车牌识别系统", mouseCallback);
    
    // 显示原图
    imshow("车牌识别系统", originalImage);
    
    //灰度预处理
    Mat gray;
    cvtColor(image,gray,COLOR_BGR2GRAY);//"convert color"（转换颜色）
    // showimg(gray);

    //高斯法去除噪声预处理
    Mat blurred;//被模糊处理的
    GaussianBlur(gray,blurred,Size(5,5),0);
    // showimg(blurred);

    //加强对比度
    Mat equalized;//被均衡化的
    equalizeHist(blurred,equalized);
    // showimg(equalized);

    //Canny边缘化预处理
    Mat edges;
    Canny(equalized,edges,100,200);//数字为边缘化阈值
    // showimg(edges);

    //边缘检测与形态学处理
    Mat morphed;//被形态学处理过的
    Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 3));//生成一个矩形结构元素
    dilate(edges, morphed, kernel);//使用矩形结构进行膨胀处理
    // showimg(morphed);

    //查找轮廓并筛选车牌
    vector<vector<Point>> contours;//轮廓的数组
    vector<Rect> candidateRects;//矩形数组
    findContours(morphed,contours,RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);
    for (const auto &contour:contours) {
        Rect rect=boundingRect(contour);//外接矩形
        float aspectRatio=float(rect.width)/float(rect.height);
        if (aspectRatio>2.0&&aspectRatio<6.0&&rect.area()>1000) {//长宽比，面积筛选矩形轮廓
            candidateRects.push_back(rect);
        }
    }
    std::sort(candidateRects.begin(), candidateRects.end(), CompareRectArea);
    if (candidateRects.size() < 1) {//这里我们筛选一个出来,可以多个
        cerr<<"ERROR"<<endl;
        return -1;
    }
    vector<Rect> rects={candidateRects[0]};
    Mat plateImg=image(rects[0]);
    Mat resized=resizeToMinWidth(plateImg,100);
    
    // 保存处理后的图像用于显示
    processedImage = resized.clone();
    
    //二值化
    Mat resizeGray,binary;//调整大小的灰度图，二值图
    cvtColor(resized,resizeGray,COLOR_BGR2GRAY);
    threshold(resizeGray,binary,0,255,THRESH_BINARY + THRESH_OTSU);
    if (mean(binary)[0]>128) bitwise_not(binary,binary);//像素平均值大于128则大部分为白色
    // showimg(binary);

    //筛选字符
    contours.clear();
    candidateRects.clear();
    findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    for (const auto &contour:contours) {
        Rect rect=boundingRect(contour);//外接矩形
        float aspect=float(rect.width)/float(rect.height);
        if (aspect>0.4&&aspect<1&&rect.area()>30&&rect.area()<600) {//长宽比，面积筛选矩形轮廓
            candidateRects.push_back(rect);
            // cout<<aspect<<endl;
        }
    }
    sort(candidateRects.begin(), candidateRects.end(), CompareRectX);
    characters.clear();
    for (const auto &rect:candidateRects) {
        characters.push_back(binary(rect).clone());
        // showimg(binary(rect));
    }

    // 读取模板图像
    vector<cv::Mat> templates;//模板
    vector<string> labels = {"N","P","F","8"};
    // vector<string> labels = {"0", "1", "2", "3", "4", "5", "6", "7",
    //     "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I",
    //     "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    //     "U", "V", "W", "X", "Y", "Z"};

    int loadedTemplates = 0;
    for (const auto& label : labels) {
        cv::Mat template_img = cv::imread("D:\\code\\vscode_opencv\\templates\\" + label + ".jpg", cv::IMREAD_GRAYSCALE);//文件名必须为templates/label.jpg
        if (!template_img.empty()) {
            cv::resize(template_img, template_img, cv::Size(32, 32));
            templates.push_back(template_img);
            loadedTemplates++;
        } else {
            cerr << "警告：无法加载模板 " << label << ".jpg" << endl;
        }
    }
    
    if (loadedTemplates == 0) {
        cerr << "错误：没有加载到任何模板！请检查templates文件夹是否存在。" << endl;
        return -1;
    }
    
    cout << "成功加载 " << loadedTemplates << " 个模板" << endl;

    //进行匹配
    recognitionResults.clear();
    for (int i=0;i<characters.size();i++) {
        Mat img=characters[i];
        string ss=recognizeChar(img,templates,labels);
        recognitionResults.push_back(ss);
    }
    
    // 创建结果图像
    resultImage = resized.clone();
    for (int i = 0; i < candidateRects.size() && i < recognitionResults.size(); i++) {
        Rect rect = candidateRects[i];
        string result = recognitionResults[i];
        putText(resultImage, result, Point(rect.x, rect.y - 5), 
                FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
    }
    
    if (!recognitionResults.empty()) {
        cout << "识别结果: ";
        for (auto x:recognitionResults){
            cout<<x<<" ";
        }
        cout<<endl;
    }
    
    // 创建控制面板
    createControlPanel();
    
    // 主循环
    cout << "使用说明：" << endl;
    cout << "1. 点击图像窗口切换显示模式" << endl;
    cout << "2. 按 'q' 退出程序" << endl;
    cout << "3. 按 '1' 显示原图" << endl;
    cout << "4. 按 '2' 显示处理后的图像" << endl;
    cout << "5. 按 '3' 显示识别结果" << endl;
    
    char key;
    while (true) {
        key = waitKey(1);
        
        if (key == 'q' || key == 27) { // q 或 ESC
            break;
        } else if (key == '1') {
            imshow("车牌识别系统", originalImage);
        } else if (key == '2') {
            imshow("车牌识别系统", processedImage);
        } else if (key == '3') {
            imshow("车牌识别系统", resultImage);
        }
    }

    return 0;
}
