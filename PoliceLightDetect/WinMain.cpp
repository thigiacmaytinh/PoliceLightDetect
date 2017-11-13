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
#include "TGMTutil.h"

int g_totalFrame;
int g_maxLightSize;
int g_minLightSize;
int g_lastFrameHasBlueLight;
int g_lastFrameHasRedLight;
int g_frameFrequency;
int g_blurSize;
bool g_debug;

cv::Scalar lowRed1, lowRed2, highRed1, highRed2;
cv::Scalar lowBlue1, lowBlue2, highBlue1, highBlue2;

cv::Mat g_lastBlueMask, g_lastRedMask;

#define INI_APP_CONFIG "Police light detect"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ParseColor(std::string str, cv::Scalar& low, cv::Scalar& high)
{
	std::vector<std::string> split = TGMTutil::SplitString(str, ',');
	if (split.size() != 6)
	{		
		return false;
	}

	low = cv::Scalar(atoi(split[0].c_str()), atoi(split[2].c_str()), atoi(split[4].c_str()));
	high = cv::Scalar(atoi(split[1].c_str()), atoi(split[3].c_str()), atoi(split[5].c_str()));
	return true;
}

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
	
	cv::inRange(matHsv, lowRed1, highRed1, maskRedLeft);
	cv::inRange(matHsv, lowRed2, highRed2, maskRedRight);

	cv::bitwise_or(maskRedLeft, maskRedRight, maskRed);
	TGMTmorphology::Dilate(maskRed, cv::MORPH_ELLIPSE, 10);
	//cv::imshow("out Red", maskRed);

	matInput.copyTo(matResult, maskRed);

	if (g_debug)
	{
		cv::imshow("red light", matResult);
	}
	return maskRed;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat DetectBlueLight(cv::Mat matInput)
{
	cv::Mat matHsv;
	cv::cvtColor(matInput, matHsv, CV_BGR2HSV);

	cv::Mat maskBlueOutside, maskBlueInside, matResult;

	cv::inRange(matHsv, lowBlue1, highBlue1, maskBlueOutside);
	cv::inRange(matHsv, lowBlue2, highBlue2, maskBlueInside);
	TGMTmorphology::Dilate(maskBlueInside, cv::MORPH_ELLIPSE, 10);

	cv::bitwise_and(maskBlueOutside, maskBlueInside, maskBlueOutside);
	matInput.copyTo(matResult, maskBlueOutside);
	if (g_debug)
	{
		cv::imshow("Blue light", matResult);
	}
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
	if (g_blurSize > 0 && g_blurSize % 2 == 1)
	{
		cv::GaussianBlur(matBlur, matBlur, cv::Size(g_blurSize, g_blurSize), 0);
	}
	
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
	//if (isBlueLightOn)
	//{
	//	g_lastBlueMask = maskBlue;
	//	g_lastFrameHasBlueLight = frameIdx;
	//	//TGMTdraw::PutText(frame, cv::Point(10, 90), BLUE, "BLUE");
	//	if (frameIdx - g_lastFrameHasRedLight <= g_frameFrequency)
	//	{
	//		isPoliceCar = true;			
	//	}
	//}	
	//if (isRedLightOn)
	//{
	//	g_lastRedMask = maskRed;
	//	g_lastFrameHasRedLight = frameIdx;
	//	//TGMTdraw::PutText(frame, cv::Point(10, 30), RED, "RED");
	//	if (frameIdx - g_lastFrameHasBlueLight <= g_frameFrequency)
	//	{
	//		isPoliceCar = true;			
	//	}
	//}
	
	if (isPoliceCar)
	{
		cv::Mat maskOut;
		TGMTdraw::PutText(frame, cv::Point(10, 30), YELLOW, "POLICE");
		cv::bitwise_or(g_lastBlueMask, g_lastRedMask, maskOut);
		std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(maskOut, 30, cv::Size(g_minLightSize, g_minLightSize),
			cv::Size(g_maxLightSize, g_maxLightSize));
		if (contours.size() > 0)
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

	g_blurSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "blur_size", 11);

	g_debug = GetTGMTConfig()->ReadValueBool(INI_APP_CONFIG, "debug");

	std::string redColor1 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "red_color1");
	std::string redColor2 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "red_color2");
	std::string blueColor1 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "blue_color1");
	std::string blueColor2 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "blue_color2");

	ParseColor(redColor1, lowRed1, highRed1);
	ParseColor(redColor2, lowRed2, highRed2);
	ParseColor(blueColor1, lowBlue1, highBlue1);
	ParseColor(blueColor2, lowBlue2, highBlue2);

	GetTGMTvideo()->OnNewFrame = OnVideoFrame;
	g_totalFrame = GetTGMTvideo()->GetAmountFrame(videoFile);


	GetTGMTvideo()->PlayVideo(videoFile, cv::Size(w, h), 0, startIdx);




	cv::waitKey();
	getchar();
	return 0;
}

