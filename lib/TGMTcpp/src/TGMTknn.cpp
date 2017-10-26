#include "TGMTknn.h"
#include "TGMTdebugger.h"
#include "TGMTfile.h"

TGMTknn* TGMTknn::m_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TGMTknn::TGMTknn()
{
#if CV_MAJOR_VERSION == 3
	knn = KNearest::create();
	knn->setIsClassifier(true);
	knn->setAlgorithmType(KNearest::Types::BRUTE_FORCE);
#else
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TGMTknn::~TGMTknn()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTknn::TrainData(std::vector<std::string> imgPaths, std::vector<int> labels, int width, int height)
{
	ASSERT(imgPaths.size() > 0 && labels.size() > 0 && imgPaths.size() == labels.size(), "SVM train data is not valid");
	START_COUNT_TIME("train_knn");
	SET_CONSOLE_TITLE("Traning knn data...");
	size_t numMats = imgPaths.size();
	
	int matArea = width * height;
	cv::Mat matData = cv::Mat(numMats, matArea, CV_32FC1);
	cv::Mat matLabels = cv::Mat(numMats, 1, CV_32FC1);

	//prepare train set
	for (size_t fileIndex = 0; fileIndex < numMats; fileIndex++)
	{

		//set label
		matLabels.at<float>(fileIndex, 0) = labels[fileIndex];

		cv::Mat matCurrent = cv::imread(imgPaths[fileIndex], CV_LOAD_IMAGE_GRAYSCALE);

		if (matCurrent.cols != width || matCurrent.rows != height)
		{
			cv::resize(matCurrent, matCurrent, cv::Size(width, height));
		}

		matCurrent = matCurrent.reshape(1, 1);
		matCurrent.row(0).copyTo(matData.row(fileIndex));
	}
	knn->train(matData, ROW_SAMPLE, matLabels);
	mIsTrained = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTknn::TrainData(std::string dirPath, int width, int height)
{
	std::vector<std::string> files = TGMTfile::GetImageFilesInDir(dirPath, true);
	if (files.size() == 0)
	{
		PrintError("Not found any image");
		return;
	}

	std::vector<std::string> validFiles;
	std::vector<int> labels;
	for (int i = 0; i < files.size(); i++)
	{
		std::string filePath = files[i];
		std::string parentDir = TGMTfile::GetParentDir(filePath);
		if (parentDir.length() > 2)
			continue;

		validFiles.push_back(filePath);
		int label = (char)parentDir[0];
		labels.push_back(label);
	}

	TrainData(validFiles, labels, width, height);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float TGMTknn::Predict(cv::Mat matInput, int width, int height)
{
	ASSERT(matInput.data, "Mat input is null");
	ASSERT(mIsTrained, "You must train KNN before use");
	ASSERT(matInput.channels() == 1, "Image input is not grayscale");
	if (matInput.cols != width || matInput.rows != height)
	{
		cv::resize(matInput, matInput, cv::Size(width, height));
	}
	
	cv::Mat matData;
	if (matInput.type() != CV_32FC1)
	{
		matData = matInput.reshape(1, 1);
		matData.convertTo(matData, CV_32FC1);
	}
	else
	{
		matData = matInput;
	}
	cv::Mat matResults(0, 0, CV_32FC1);
	return knn->findNearest(matData, knn->getDefaultK() , matResults);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTknn::SaveData(std::string fileName)
{
	knn->save(fileName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTknn::LoadData(std::string fileName)
{
	knn = cv::Algorithm::load<KNearest>(fileName);
	ASSERT(knn != nullptr, "Can not load file: %s", fileName.c_str());
	mIsTrained = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float TGMTknn::Predict(std::string filePath, int width, int height)
{
	cv::Mat mat = cv::imread(filePath, CV_LOAD_IMAGE_GRAYSCALE);
	return Predict(mat, width, height);
}