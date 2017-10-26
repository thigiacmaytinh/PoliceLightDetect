#pragma once
#include "stdafx.h"

using namespace cv::ml;

#define GetTGMTknn TGMTknn::GetInstance

class TGMTknn
{
	bool mIsTrained = false;

#if CV_MAJOR_VERSION == 3
	cv::Ptr<KNearest> knn;
#else
	cv::KNearest    *knn;
#endif

	static TGMTknn* m_instance;
public:
	TGMTknn();
	~TGMTknn();

	static TGMTknn* GetInstance()
	{
		if (!m_instance)
			m_instance = new TGMTknn();
		return m_instance;
	}

	void TrainData(std::vector<std::string> imgPaths, std::vector<int> labels, int width, int height);
	void TrainData(std::string dirPath, int width, int height);

	float Predict(cv::Mat matInput, int width, int height);
	float Predict(std::string imgPath, int width, int height);
	void SaveData(std::string fileName);
	void LoadData(std::string fileName);
};

