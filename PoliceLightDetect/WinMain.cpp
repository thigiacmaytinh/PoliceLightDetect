// TGMTtemplate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include "TGMTConfig.h"
#include "TGMTvideo.h"
#include "TGMTcolor.h"
#include "TGMTblob.h"
#include "TGMTdraw.h"
#include "TGMTdebugger.h"
#include "TGMTimage.h"
#include "TGMTmorphology.h"
#include "TGMTcontour.h"
#include "TGMTshape.h"
#include "TGMTbrightness.h"

int g_totalFrame;
int g_maxLightSize;
int g_minLightSize;
int g_lastFrameHasBlueLight;
int g_lastFrameHasRedLight;
int g_frameFrequency;
cv::Mat g_lastBlueMask, g_lastRedMask;

#define INI_APP_CONFIG "Police light detect"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DetectWhiteLight(cv::Mat matInput)
{
	cv::Mat matGray;
	cv::cvtColor(matInput, matGray, CV_BGR2GRAY);
	

	//cv::equalizeHist(matGray, matGray);
	cv::Mat matBinary;
	cv::threshold(matGray, matBinary, 250, 255, CV_THRESH_BINARY);
	cv::imshow("binary", matBinary);
	auto blobs = TGMTblob::FindBlobs(matBinary, cv::Size(40,20));
	TGMTblob::DrawBoundingRects(matInput, blobs, cv::Point(0,0), RED, 2);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat DetectBrightSpot(cv::Mat matInput)
{
	cv::Mat matGray = TGMTimage::ConvertToGray(matInput);
	cv::Mat matBin;
	cv::threshold(matGray, matBin, 240, 255, cv::THRESH_BINARY);


	//TGMTmorphology::Erode(matBin, cv::MORPH_RECT, 2);
	TGMTmorphology::Dilate(matBin, cv::MORPH_ELLIPSE, 18);

	

	std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(matBin, true, cv::Size(g_minLightSize, g_minLightSize), 
		cv::Size(g_maxLightSize, g_maxLightSize));
	//std::vector<TGMTshape::Circle> circles = TGMTcontour::GetEnclosingCircle(contours);

	//matInput = TGMTdraw::DrawCircles(matInput, circles, RED, 2);
	matInput = TGMTcontour::DrawBoundingRects(matInput, contours, UNDEFINED_COLOR, 2);
	cv::imshow("mat white", matInput);
	return matBin;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat DetectRedLight(cv::Mat matInput)
{
	cv::Mat matHsv;
	cv::cvtColor(matInput, matHsv, CV_BGR2HSV);

	cv::Mat maskRedLeft, maskRedRight, maskRed, matResult;
	
	cv::Scalar lowerRedLeft = cv::Scalar(0, 130, 171);
	cv::Scalar higherRedLeft = cv::Scalar(14, 214, 255);
	cv::inRange(matHsv, lowerRedLeft, higherRedLeft, maskRedLeft);

	cv::Scalar lowerRedRight = cv::Scalar(158, 111, 201);
	cv::Scalar higherRedRight = cv::Scalar(179, 163, 255);
	cv::inRange(matHsv, lowerRedRight, higherRedRight, maskRedRight);

	cv::bitwise_or(maskRedLeft, maskRedRight, maskRed);
	TGMTmorphology::Dilate(maskRed, cv::MORPH_ELLIPSE, 10);
	//cv::imshow("out Red", maskRed);

	matInput.copyTo(matResult, maskRed);

	//cv::imshow("Red", matResult);
	return maskRed;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat DetectBlueLight(cv::Mat matInput)
{
	cv::Mat matHsv;
	cv::cvtColor(matInput, matHsv, CV_BGR2HSV);

	cv::Mat maskBlueOutside, maskBlueInside, matResult;

	cv::Scalar lowerBlueOutside = cv::Scalar(76,0,167);
	cv::Scalar higherBlueOutside = cv::Scalar(121,255,255);
	cv::inRange(matHsv, lowerBlueOutside, higherBlueOutside, maskBlueOutside);

	cv::Scalar lowerBlueInside = cv::Scalar(108, 85, 129);
	cv::Scalar higherBlueInside = cv::Scalar(118, 255, 255);
	cv::inRange(matHsv, lowerBlueInside, higherBlueInside, maskBlueInside);
	TGMTmorphology::Dilate(maskBlueInside, cv::MORPH_ELLIPSE, 10);

	cv::bitwise_and(maskBlueOutside, maskBlueInside, maskBlueOutside);
	matInput.copyTo(matResult, maskBlueOutside);
	return maskBlueOutside;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsBlueLightOn(cv::Mat matMask)
{
	std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(matMask, true, cv::Size(g_minLightSize, g_minLightSize),
		cv::Size(g_maxLightSize, g_maxLightSize));
	
	return contours.size() > 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsRedLightOn(cv::Mat matMask)
{
	std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(matMask, true, cv::Size(g_minLightSize, g_minLightSize),
		cv::Size(g_maxLightSize, g_maxLightSize));

	return contours.size() > 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OnVideoFrame(cv::Mat frame)
{
	cv::Mat matBlur = frame.clone();
	TGMTbrightness::EqualizeHist(matBlur);
	int blurSize = 17;
	cv::GaussianBlur(matBlur, matBlur, cv::Size(blurSize, blurSize), 0);
	//cv::imshow("Input", frame);
	//cv::imshow("blur", matBlur);
	int frameIdx = GetTGMTvideo()->m_frameCount;

	cv::Mat maskBlue = DetectBlueLight(matBlur);
	cv::Mat maskRed = DetectRedLight(matBlur);
	

	bool isBlueLightOn = IsBlueLightOn(maskBlue);
	bool isRedLightOn = IsRedLightOn(maskRed);
	bool isPoliceCar = false;

	if (isBlueLightOn && isRedLightOn)
	{
		isPoliceCar = true;
		g_lastFrameHasBlueLight = frameIdx;
		g_lastFrameHasRedLight = frameIdx;
	}
	if (isBlueLightOn)
	{
		g_lastBlueMask = maskBlue;
		g_lastFrameHasBlueLight = frameIdx;
		//TGMTdraw::PutText(frame, cv::Point(10, 90), BLUE, "BLUE");
		if (frameIdx - g_lastFrameHasRedLight <= g_frameFrequency)
		{
			isPoliceCar = true;			
		}
	}	
	if (isRedLightOn)
	{
		g_lastRedMask = maskRed;
		g_lastFrameHasRedLight = frameIdx;
		//TGMTdraw::PutText(frame, cv::Point(10, 30), RED, "RED");
		if (frameIdx - g_lastFrameHasBlueLight <= g_frameFrequency)
		{
			isPoliceCar = true;			
		}
	}
	
	if (isPoliceCar)
	{
		cv::Mat maskOut;
		TGMTdraw::PutText(frame, cv::Point(10, 30), YELLOW, "POLICE");
		cv::bitwise_or(g_lastBlueMask, g_lastRedMask, maskOut);
		std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(maskOut, 30, cv::Size(g_minLightSize, g_minLightSize),
			cv::Size(g_maxLightSize, g_maxLightSize));
		{
			TGMTcontour::Contour biggestContour = TGMTcontour::GetBiggestContour(contours);
			frame = TGMTcontour::DrawBoundingRect(frame, biggestContour, YELLOW, 2);
		}
	}
	
	cv::imshow("Ouput", frame);
	std::cout << "\r" << "Frame: " << GetTGMTvideo()->m_frameCount + 1<< " / " << g_totalFrame;

	cv::waitKey(1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	if (!GetTGMTConfig()->LoadSettingFromFile("PoliceLight.ini"))
	{
		PrintError("Can not load setting: PoliceLight.json");
		return 0;
	}
	
	std::string videoFile = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "video");

	int w = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "input_width");
	int h = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "input_height");
	int startIdx = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "start_frame");
	g_maxLightSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "max_light_size");
	g_minLightSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "min_light_size");

	g_frameFrequency = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "frame_frequency");

	GetTGMTvideo()->OnNewFrame = OnVideoFrame;
	g_totalFrame = GetTGMTvideo()->GetAmountFrame(videoFile);


	GetTGMTvideo()->PlayVideo(videoFile, cv::Size(w, h), 0, startIdx);




	cv::waitKey();
	getchar();
	return 0;
}

