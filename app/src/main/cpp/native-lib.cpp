#include <jni.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

using namespace cv;
using namespace std;

struct Sample {
    vector<float> descriptor;
    string label; // "circle", "square", "triangle"
};

vector<Sample> dataset;
bool datasetLoaded = false;

vector<string> classes = {"circulo", "cuadrado", "triangulo"};

int classIndex(const string& label) {
    for (int i = 0; i < classes.size(); i++) {
        if (classes[i] == label) return i;
    }
    return -1;
}
int confusion[3][3] = {0};

double accuracy() {
    int correct = 0, total = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            total += confusion[i][j];
            if (i == j) correct += confusion[i][j];
        }
    }
    return total > 0 ? (double)correct / total : 0;
}


double euclidean(const vector<float>& a, const vector<float>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        double d = a[i] - b[i];
        sum += d * d;
    }
    return sqrt(sum);
}


int idx(const string& s) {
    if (s == "circulo") return 0;
    if (s == "cuadrado") return 1;
    if (s == "triangulo") return 2;
    return -1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_appshapesignature_MainActivity_updateConfusionMatrix(
        JNIEnv* env,
        jobject,
        jstring real,
        jstring pred
) {
    string r = env->GetStringUTFChars(real, 0);
    string p = env->GetStringUTFChars(pred, 0);

    int i = idx(r);
    int j = idx(p);

    if (i >= 0 && j >= 0) {
        confusion[i][j]++;
    }
}

string confusionToString() {
    string s = "Matriz de Confusión\n";
    s += "      C   Q   T\n";

    for (int i = 0; i < 3; i++) {
        s += classes[i].substr(0,1) + " | ";
        for (int j = 0; j < 3; j++) {
            s += to_string(confusion[i][j]) + " ";
        }
        s += "\n";
    }
    return s;
}


vector<float> computeDescriptor(const Mat& inputRGBA) {

    Mat gray, bin;
    cvtColor(inputRGBA, gray, COLOR_RGBA2GRAY);

    adaptiveThreshold(
            gray, bin, 255,
            ADAPTIVE_THRESH_GAUSSIAN_C,
            THRESH_BINARY, 11, 2
    );

    bitwise_not(bin, bin);

    vector<vector<Point>> contours;
    findContours(bin, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return {};

    int maxIdx = 0;
    double maxArea = 0;
    for (int i = 0; i < contours.size(); i++) {
        double a = contourArea(contours[i]);
        if (a > maxArea) {
            maxArea = a;
            maxIdx = i;
        }
    }

    vector<Point> contour = contours[maxIdx];
    Moments m = moments(contour);
    if (m.m00 == 0) return {};

    double xc = m.m10 / m.m00;
    double yc = m.m01 / m.m00;

    vector<Vec2f> signature;
    for (auto& p : contour) {
        signature.push_back(Vec2f(p.x - xc, p.y - yc));
    }

    Mat input(signature);
    input = input.reshape(1, 1);
    input.convertTo(input, CV_32F);

    Mat dftResult;
    dft(input, dftResult, DFT_COMPLEX_OUTPUT);

    Vec2f f1 = dftResult.at<Vec2f>(0, 1);
    float mag1 = sqrt(f1[0]*f1[0] + f1[1]*f1[1]);
    if (mag1 == 0) return {};

    vector<float> descriptor;
    for (int i = 1; i < min(32, dftResult.cols); i++) {
        Vec2f f = dftResult.at<Vec2f>(0, i);
        float mag = sqrt(f[0]*f[0] + f[1]*f[1]);
        descriptor.push_back(mag / mag1);
    }

    return descriptor;
}

void loadDataset(JNIEnv* env, jobject assetManagerObj) {
    if (datasetLoaded) return;

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManagerObj);
    AAssetDir* dir = AAssetManager_openDir(mgr, "dataset");

    const char* filename;
    while ((filename = AAssetDir_getNextFileName(dir)) != nullptr) {

        string name(filename);
        string label;

        if (name[0] == 'c') label = "circulo";
        else if (name[0] == 'q') label = "cuadrado";
        else if (name[0] == 't') label = "triangulo";
        else continue;

        string fullPath = string("dataset/") + filename;
        AAsset* asset = AAssetManager_open(mgr, fullPath.c_str(), AASSET_MODE_BUFFER);

        size_t len = AAsset_getLength(asset);
        vector<uchar> buffer(len);
        AAsset_read(asset, buffer.data(), len);
        AAsset_close(asset);

        Mat img = imdecode(buffer, IMREAD_UNCHANGED);
        if (img.empty()) continue;

        vector<float> desc = computeDescriptor(img);
        if (!desc.empty()) {
            dataset.push_back({desc, label});
        }
    }

    AAssetDir_close(dir);
    datasetLoaded = true;
}



extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_appshapesignature_MainActivity_classifyBitmapRGBA(
        JNIEnv* env,
        jobject thiz,
        jint w,
        jint h,
        jintArray pixels,
        jobject assetManager
) {
    // 1. Obtener pixeles desde Kotlin
    jint* px = env->GetIntArrayElements(pixels, nullptr);

    // 2. Crear imagen RGBA
    Mat img(h, w, CV_8UC4, px);
    Mat imgCopy = img.clone(); // copiar para seguridad

    env->ReleaseIntArrayElements(pixels, px, 0);


    loadDataset(env, assetManager);

    if (dataset.empty()) {
        return env->NewStringUTF("Dataset vacío");
    }

    // 3. Convertir a gris
    Mat gray;
    cvtColor(imgCopy, gray, COLOR_RGBA2GRAY);

    // 4. Binarización adaptativa
    Mat bin;
    adaptiveThreshold(
            gray,
            bin,
            255,
            ADAPTIVE_THRESH_GAUSSIAN_C,
            THRESH_BINARY,
            11,
            2
    );

    // 5. Invertir (figura blanca, fondo negro)
    bitwise_not(bin, bin);

    // 6. Buscar contornos
    vector<vector<Point>> contours;
    findContours(bin, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return env->NewStringUTF("No se detectó figura");
    }

    // 7. Tomar el contorno más grande
    int maxIdx = 0;
    double maxArea = 0;

    for (int i = 0; i < contours.size(); i++) {
        double a = contourArea(contours[i]);
        if (a > maxArea) {
            maxArea = a;
            maxIdx = i;
        }
    }

    vector<Point> contour = contours[maxIdx];

    Moments m = moments(contour);

    if (m.m00 == 0) {
        return env->NewStringUTF("Error centroide");
    }

    double xc = m.m10 / m.m00;
    double yc = m.m01 / m.m00;

    vector<Vec2f> signature;

    for (const Point& p : contour) {
        float real = p.x - xc;
        float imag = p.y - yc;
        signature.push_back(Vec2f(real, imag));
    }

    // 11. DFT de la firma
    Mat input(signature);
    input = input.reshape(1, 1);   // 1 fila
    input.convertTo(input, CV_32F);

    Mat dftResult;
    dft(input, dftResult, DFT_COMPLEX_OUTPUT);

    // 12. Normalización usando el primer armónico
    Vec2f f1 = dftResult.at<Vec2f>(0, 1);
    float mag1 = sqrt(f1[0]*f1[0] + f1[1]*f1[1]);

    if (mag1 == 0) {
        return env->NewStringUTF("Error normalización");
    }

    vector<float> descriptor;

    for (int i = 1; i < min(32, dftResult.cols); i++) {
        Vec2f f = dftResult.at<Vec2f>(0, i);
        float mag = sqrt(f[0]*f[0] + f[1]*f[1]);
        descriptor.push_back(mag / mag1);
    }



    vector<float> inputDesc = computeDescriptor(imgCopy);

    double bestDist = 1e9;
    string bestLabel = "Desconocido";


    for (auto& s : dataset) {
        double d = euclidean(inputDesc, s.descriptor);
        if (d < bestDist) {
            bestDist = d;
            bestLabel = s.label;
        }
    }

    return env->NewStringUTF(bestLabel.c_str());

}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_appshapesignature_MainActivity_getMetrics(
        JNIEnv* env,
        jobject
) {
    double acc = accuracy();
    string accStr = "Accuracy: " + to_string(acc * 100.0) + "%";

    string out = confusionToString() + "\n" + accStr;
    return env->NewStringUTF(out.c_str());
}

