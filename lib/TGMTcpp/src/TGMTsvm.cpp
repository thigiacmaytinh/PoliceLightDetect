#include "TGMTsvm.h"
#include "TGMTdebugger.h"
#include "TGMTfile.h"

TGMTsvm* TGMTsvm::m_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TGMTsvm::TGMTsvm()
{
#if CV_MAJOR_VERSION == 3
	svm = SVM::create();
#else
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TGMTsvm::~TGMTsvm()
{
#if CV_MAJOR_VERSION == 3
	svm.release();
#else
	svm.~CvSVM();
#endif
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTsvm::TrainData(std::vector<std::string> imgPaths, std::vector<int> labels, int width, int height)
{
	ASSERT(imgPaths.size() > 0 && labels.size() > 0 && imgPaths.size() == labels.size(), "SVM train data is not valid");

	START_COUNT_TIME("train_svm");
	SET_CONSOLE_TITLE("Traning svm data...");
	size_t numMats = imgPaths.size();
	
	int matArea = width * height;
	cv::Mat matData = cv::Mat(numMats, matArea, CV_32FC1);
	cv::Mat matLabels = cv::Mat(numMats, 1, CV_32S);

	//prepare train set
	for (size_t fileIndex = 0; fileIndex < numMats; fileIndex++)
	{
		
		//set label
		matLabels.at<int>(fileIndex, 0) = labels[fileIndex];

		cv::Mat matCurrent = cv::imread(imgPaths[fileIndex], CV_LOAD_IMAGE_GRAYSCALE);
		cv::threshold(matCurrent, matCurrent, 10, 255, CV_THRESH_BINARY);
		if (matCurrent.cols != width || matCurrent.rows != height)
		{
			cv::resize(matCurrent, matCurrent, cv::Size(width, height));
		}

		matCurrent = matCurrent.reshape(1, 1);
		matCurrent.row(0).copyTo(matData.row(fileIndex));
	}

#if CV_MAJOR_VERSION == 3
	svm->setType(SVM::C_SVC);
	svm->setKernel(SVM::LINEAR);
	svm->setTermCriteria(cv::TermCriteria(cv::TermCriteria::MAX_ITER, 100, 1e-6));
	svm->train(matData, ROW_SAMPLE, matLabels);
#else
	CvSVMParams params;
	params.svm_type = CvSVM::C_SVC;
	params.kernel_type = CvSVM::LINEAR;
	params.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, 100, 1e-6);

	svm.train(matData, matLabels, cv::Mat(), cv::Mat(), params);
#endif

	
	mIsTrained = true;
	STOP_AND_PRINT_COUNT_TIME("train_svm");
	PrintSuccess("Trained %d images", numMats);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTsvm::TrainData(std::string dirPath, int width, int height)
{
	std::vector<std::string> files = TGMTfile::GetImageFilesInDir(dirPath, true);
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

float TGMTsvm::Predict(cv::Mat matInput, int width, int height)
{
	ASSERT(matInput.data, "Mat input is null");	
	ASSERT(mIsTrained, "You must train SVM before use");
	ASSERT(matInput.channels() == 1, "Image input is not grayscale");
	if (matInput.cols != width || matInput.rows != height)
	{
		cv::resize(matInput, matInput, cv::Size(width, height));
	}
	
	cv::Mat matData;
	if (matInput.type() != CV_32FC1)
	{
		matData = matInput.reshape(1, 1);// cv::Mat(1, matArea, CV_32FC1);
	}
	else
	{
		matData = matInput;
	}

#if CV_MAJOR_VERSION == 3
	matData.convertTo(matData, CV_32FC1);
	return svm->predict(matData);
#else
	return svm.predict(matData);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTsvm::SaveData(std::string filePath)
{
#if CV_MAJOR_VERSION == 3
	svm->save(filePath.c_str());
#else
	svm.save(filePath.c_str());
#endif
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TGMTsvm::LoadData(std::string filePath)
{
	ASSERT(TGMTfile::FileExist(filePath), "File svm \'%s\' does not exist", filePath.c_str());
#if CV_MAJOR_VERSION == 3
	svm = StatModel::load<SVM>(filePath);
#else
	svm.load(filePath.c_str());
#endif
	
	mIsTrained = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float TGMTsvm::Predict(std::string filePath, int width, int height)
{
	cv::Mat mat = cv::imread(filePath, CV_LOAD_IMAGE_GRAYSCALE);
	return Predict(mat, width, height);
}