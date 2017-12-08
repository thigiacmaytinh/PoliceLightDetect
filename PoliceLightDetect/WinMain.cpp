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
int g_lighDistance;
bool g_debug;
int g_lastFrameDetectd;

cv::Scalar lowRed1, lowRed2, highRed1, highRed2;
cv::Scalar lowBlue1, lowBlue2, highBlue1, highBlue2;

cv::Mat g_lastBlueMask, g_lastRedMask;

#define INI_APP_CONFIG "Police light detect"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//đọc giá trị setting
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

//phát hiện vị trí đèn trắng
cv::Mat DetectWhiteLight(cv::Mat frame, cv::Mat mask)
{
	cv::Mat matGray;
	frame.copyTo(matGray, mask);

	cv::cvtColor(matGray, matGray, CV_BGR2GRAY);

	cv::Mat matBinary;
	cv::threshold(matGray, matBinary, 250, 255, CV_THRESH_BINARY);
	
	TGMTmorphology::Erode(matBinary, cv::MORPH_RECT, 31);
	//cv::imshow("binary", matBinary);

	cv::bitwise_and(mask, matBinary, mask);
	
	cv::Mat matResult;

	frame.copyTo(matResult, mask);
	return matResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//khi tìm được đèn xanh/đỏ thì cần kiểm tra thêm 1 lần nữa xem có màu trắng ở giữa không 
cv::Mat CheckWhiteLightInside(cv::Mat frame, cv::Mat mask)
{
	//tìm tất cả các contour
	std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(mask, 30, cv::Size(g_minLightSize, g_minLightSize),
		cv::Size(g_maxLightSize, g_maxLightSize));
		
	//chuyển sang ảnh xám
	cv::Mat matGray = TGMTimage::ConvertToGray(frame);

	//cân bằng sáng
	cv::equalizeHist(matGray, matGray);
	cv::Mat matWhiteOnly;

	//phân ngưỡng để lấy vị trí đèn màu trắng (là vùng sáng nhất)
	cv::threshold(matGray, matWhiteOnly, 250, 255, CV_THRESH_BINARY);
	
	cv::Mat maskJoin = cv::Mat::zeros(frame.size(), CV_8U);
	for (int i = 0; i < contours.size(); i++)
	{
		TGMTcontour::Contour con = contours[i];
		cv::Rect rect = cv::boundingRect(con);
		cv::Mat matRoi = matWhiteOnly(rect);

		//đếm số pixel màu trắng
		if (cv::countNonZero(matRoi) > 20)
		{
			mask(rect).copyTo(maskJoin(rect));
			matWhiteOnly(rect).copyTo(maskJoin(rect));
		}
	}

	return maskJoin;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//trả về ảnh chỉ chứa đèn đỏ
cv::Mat DetectMaskRed(cv::Mat frame)
{
	cv::Mat matHsv;

	//chuyển sang màu hsv
	cv::cvtColor(frame, matHsv, CV_BGR2HSV);

	cv::Mat maskRedLeft, maskRedRight, maskRed;
	
	//giới hạn màu
	cv::inRange(matHsv, lowRed1, highRed1, maskRedLeft);
	cv::inRange(matHsv, lowRed2, highRed2, maskRedRight);

	//tổng hợp mask
	cv::bitwise_or(maskRedLeft, maskRedRight, maskRed);

	//kiểm tra xem vùng nào có đèn màu trắng bên trong thì đó chính xác là đèn đỏ
	maskRed = CheckWhiteLightInside(frame, maskRed);


	if (g_debug)
	{
		cv::Mat matResult;
		frame.copyTo(matResult, maskRed);
		cv::imshow("red light", matResult);
	}
	return maskRed;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//trả về ảnh chỉ chứa đèn xanh
cv::Mat DetectMaskBlue(cv::Mat frame)
{
	cv::Mat matHsv;
	//chuyển sang màu hsv
	cv::cvtColor(frame, matHsv, CV_BGR2HSV);

	cv::Mat maskBlueOutside, maskBlueInside, mask;

	//giới hạn màu
	cv::inRange(matHsv, lowBlue1, highBlue1, maskBlueOutside);
	cv::inRange(matHsv, lowBlue2, highBlue2, maskBlueInside);
	mask = maskBlueInside;
	
	//tổng hợp mask
	cv::bitwise_or(maskBlueOutside, maskBlueInside, mask);

	//kiểm tra xem vùng nào có đèn màu trắng bên trong thì đó chính xác là đèn xanh
	mask = CheckWhiteLightInside(frame, mask);
	if (g_debug)
	{
		cv::Mat matResult;
		frame.copyTo(matResult, mask);
		cv::imshow("Blue light", matResult);
	}
	return mask;
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

//mở rộng mask dựa trên thông số khoảng cách giữa 2 đèn xanh & đỏ
cv::Mat ExpandMask(cv::Mat mask)
{
	if (g_lighDistance < 1)
		return mask;

	cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
		cv::Size(2 * g_lighDistance + 1, 2 * g_lighDistance + 1),
		cv::Point(g_lighDistance, g_lighDistance));

	//Áp dụng dilation (mở rộng)
	cv::dilate(mask.clone(), mask, element);
	return mask;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//mỗi khi đọc 1 frame thì gọi đến hàm này
void OnVideoFrame(cv::Mat frame)
{
	cv::Mat matBlur = frame.clone();

	//cân bằng sáng cho ảnh
	TGMTbrightness::EqualizeHist(matBlur);

	//nếu giá trị blur đúng (lớn hơn 0 và số lẻ) thì làm mờ ảnh
	if (g_blurSize > 0 && g_blurSize % 2 == 1)
	{
		cv::blur(matBlur, matBlur, cv::Size(g_blurSize, g_blurSize));
	}

	int frameIdx = GetTGMTvideo()->m_frameCount;

	//phát hiện mask của đèn màu xanh và đỏ
	cv::Mat maskBlue = DetectMaskBlue(matBlur);
	cv::Mat maskRed = DetectMaskRed(matBlur);

	//tổng hợp 2 mask xanh và đỏ
	cv::Mat maskRedBlue;
	cv::bitwise_or(maskRed, maskBlue, maskRedBlue);

	//mở rộng mask để tìm điểm giao nhau của 2 đèn xanh/đỏ
	maskRed = ExpandMask(maskRed);
	maskBlue = ExpandMask(maskBlue);

	cv::Mat maskResult, matResult;
	cv::bitwise_and(maskRed, maskBlue, maskResult);
	maskResult = ExpandMask(maskResult);

	//hiển thị vị trí đèn xanh & đỏ
	if (g_debug)
	{
		cv::imshow("mask result", maskResult);
		cv::Mat matBlueAndRed;
		frame.copyTo(matBlueAndRed, maskRedBlue);
		cv::imshow("matBlueAndRed", matBlueAndRed);
	}	


	bool isPoliceCar = false;
	//đếm số blob trong mask 
	auto blobs = TGMTblob::FindBlobs(maskResult, cv::Size(g_minLightSize, g_minLightSize), cv::Size(g_maxLightSize, g_maxLightSize));



	if (blobs.size() > 0)
	{
		isPoliceCar = true;
		g_lastFrameHasBlueLight = frameIdx;
		g_lastFrameHasRedLight = frameIdx;

		//vẽ hình chữ nhật bao lấy blob
		TGMTblob::DrawBoundingRects(frame, blobs, cv::Point(0,0), YELLOW, 2);


		cv::Mat maskOut;

		//vẽ khung hình chữ nhật bên ngoài video khi phát hiện xe
		TGMTdraw::DrawRectangle(frame, cv::Rect(0, 0, frame.cols, frame.rows), PURPLE, 5);
		
		cv::bitwise_or(g_lastBlueMask, g_lastRedMask, maskOut);
		std::vector<TGMTcontour::Contour> contours = TGMTcontour::FindContours(maskOut, 30, cv::Size(g_minLightSize, g_minLightSize),
			cv::Size(g_maxLightSize, g_maxLightSize));

		//chỉ vẽ contour lớn nhất
		if (contours.size() > 0)
		{
			TGMTcontour::Contour biggestContour = TGMTcontour::GetBiggestContour(contours);
			frame = TGMTcontour::DrawBoundingRect(frame, biggestContour, YELLOW, 2);
		}

		g_lastFrameDetectd = frameIdx;
	}
	
	//vẽ thứ tự khung hình
	TGMTdraw::PutText(frame, cv::Point(10, 20), BLUE,0.6, "%d/%d", frameIdx, g_totalFrame);

	cv::imshow("Ouput", frame);
	std::cout << "\r" << "Frame: " << GetTGMTvideo()->m_frameCount + 1<< " / " << g_totalFrame;

	cv::waitKey(1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	//load setting từ file config.ini
	if (!GetTGMTConfig()->LoadSettingFromFile("PoliceLight.ini"))
	{
		PrintError("Can not load setting: PoliceLight.json");
		return 0;
	}
	
	//đọc video file
	std::string videoFile = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "video");

	int w = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "input_width");
	int h = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "input_height");
	int startIdx = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "start_frame");
	g_maxLightSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "max_light_size");
	g_minLightSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "min_light_size");

	g_frameFrequency = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "frame_frequency");

	g_blurSize = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "blur_size", 11);

	//nếu giá trị debug bằng true thì chương trình sẽ hiện các bước thực hiện
	g_debug = GetTGMTConfig()->ReadValueBool(INI_APP_CONFIG, "debug");
	g_lighDistance = GetTGMTConfig()->ReadValueInt(INI_APP_CONFIG, "light_distance");

	std::string redColor1 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "red_color1");
	std::string redColor2 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "red_color2");
	std::string blueColor1 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "blue_color1");
	std::string blueColor2 = GetTGMTConfig()->ReadValueString(INI_APP_CONFIG, "blue_color2");

	//parse các giá trị màu xanh & đỏ
	bool parseResult = true;
	parseResult &= ParseColor(redColor1, lowRed1, highRed1);
	parseResult &= ParseColor(redColor2, lowRed2, highRed2);
	parseResult &= ParseColor(blueColor1, lowBlue1, highBlue1);
	parseResult &= ParseColor(blueColor2, lowBlue2, highBlue2);
	if (!parseResult)
	{
		PrintError("Load color value failed");
		return 0;
	}

	//đếm tổng số frame của video để xuất ra màn hình
	GetTGMTvideo()->OnNewFrame = OnVideoFrame;
	g_totalFrame = GetTGMTvideo()->GetAmountFrame(videoFile);

	//play video
	GetTGMTvideo()->PlayVideo(videoFile, cv::Size(w, h), 0, startIdx);




	cv::waitKey();
	getchar();
	return 0;
}

