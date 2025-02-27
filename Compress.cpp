#include "pch.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream> 
#include <math.h>
#include <string>
#include <stdlib.h>  
#include <filesystem>
#include <string>
#include <regex>
#include <map>
#include <algorithm> 
#include <fstream>
#include <process.h>



using namespace cv;
using namespace std;
namespace fs = filesystem;

#define C1 (float) (0.01 * 255 * 0.01  * 255)
#define C2 (float) (0.03 * 255 * 0.03 * 255)
#define C3 (float) C2/2
#define NUM_THREADS 2;
string basePath = "images/";

struct dimension {
	string name;
	int height;
	int width;
	int blockheight;
	int blockwidth;
};
vector<dimension> resolutions;

void fillResolutions() {
	dimension R4K = {"4K", 3840, 2160, 6, 2};
	resolutions.push_back(R4K);
	dimension R3K = {"3K", 3000, 2000, 5, 2};
	resolutions.push_back(R3K);
	dimension R1K = {"1080p", 2048, 1080, 6, 2 };
	resolutions.push_back(R1K);
	dimension R720p = {"720p", 1280, 720, 6, 2 };
	resolutions.push_back(R720p);
	dimension R480p = {"480p", 544, 480, 4, 3};
	resolutions.push_back(R480p);
}

std::vector<std::string> explode(std::string const & s, char delim)
{
	std::vector<std::string> result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim);){
		result.push_back(std::move(token));
	}

	return result;
}

void copyFile(string source, string destination)
{
	std::ifstream src(source, std::ios::binary);
	std::ofstream dst(destination, std::ios::binary);
	dst << src.rdbuf();
}
Mat* normalizeLabValues(Mat image[]) {
	int rows = image[0].rows;
	int cols = image[0].cols;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			// L
			Scalar intensity = image[0].at<uchar>(i, j);
			Scalar normalized_intensity = intensity.val[0] * 100 / 256;
			image[0].at<uchar>(i, j) = normalized_intensity.val[0];
			
			// a
			intensity = image[1].at<uchar>(i, j);
			normalized_intensity = intensity.val[0] - 126;
			image[1].at<uchar>(i, j) = normalized_intensity.val[0];
			
			// b
			intensity = image[2].at<uchar>(i, j);
			normalized_intensity = intensity.val[0] - 126;
			image[2].at<uchar>(i, j) = normalized_intensity.val[0];
		}
	}


	image[0].convertTo(image[0], CV_64FC3);
	image[1].convertTo(image[1], CV_64FC3);
	image[2].convertTo(image[2], CV_64FC3);
	
	return image;
}
Mat applyFilter(string path, string filter, float intensity) {
	Mat in = imread(path);
	Mat out;
	double scale = 1;
	if (!filter.compare("blue")) { // Blu
		out = in + intensity;
	}else{ // Saturazione
		in.convertTo(out, CV_8UC1, scale, intensity);
	}
	return out;
}

bool lowerQuality(int colorSpace) {

	regex number("(.+)(_O.tif)");
	string originalPath = "images/originals/";

	for (auto& p : fs::directory_iterator(originalPath)) {
		string filePath = p.path().u8string();
		string fileName = explode(explode(filePath, '/').back(),'.')[0];

		if (regex_match(filePath, number)) {
			Mat imageOriginal = imread(filePath);
			
			for (dimension resolution : resolutions){
				cout << "Resizing image " << fileName << " to " << resolution.name << endl;
				Size size(resolution.height, resolution.width);
				Mat resizedImage;
				resize(imageOriginal, resizedImage, size);
				string newPath = basePath + "resized/" + resolution.name + "/" + fileName + ".tif";

				if (resolution.name.compare("4K") == 0) 
					copyFile(filePath, newPath);
				else
					imwrite(newPath, resizedImage);
				int nameCounter = 0;
				for (int intensity = 6; intensity <= 56; intensity += 25) {
					cout << "Saturation filter image " << fileName << " with intensity " << intensity << endl;
					Mat imageFiltered = applyFilter(newPath, "saturation", intensity);
					string newPathFilteredSaturation = basePath + "resized/" + resolution.name + "/filters/" + explode(fileName, '_')[0] + "_B" + to_string(nameCounter) + ".tif";

					imwrite(newPathFilteredSaturation, imageFiltered);


					cout << "Blue filter image " << fileName << " with intensity " << intensity << endl;
					imageFiltered = applyFilter(newPath, "blue", intensity);
					string newPathFilteredBlue = basePath + "resized/" + resolution.name + "/filters/" + explode(fileName, '_')[0] + "_B" + to_string(nameCounter+3) + ".tif";

					imwrite(newPathFilteredBlue, imageFiltered);
		
					nameCounter++;
				}
			}
			
			
			
	
		}
	}
	return true;
}
string basePathLab = "D:/dataset/";
void convertToLab() {
		for (dimension resolution : resolutions) {
			cout << "Converting images in resized/" << resolution.name << "/filters to CIELAB" << endl;
			string command = "mogrify -colorspace Lab -verbose " + basePathLab + "resized/" + resolution.name + "/filters/*.tif";
			const char* c = command.c_str();
			system((const char*)c);
			cout << command << endl << endl;
			/*cout << "Converting images in resized/" << resolution.name << "/saturated to CIELAB" << endl;
			command = "mogrify -colorspace Lab -verbose " + basePathLab + "resized/" + resolution.name + "/saturated/*.tif";
			c = command.c_str();
			system((const char*)c);*/
			cout << "Converting images in resized/" << resolution.name << " to CIELAB" << endl;
			command = "mogrify -colorspace Lab -verbose " + basePathLab + "resized/" + resolution.name + "/*.tif";
			c = command.c_str();
			system((const char*)c);
		}
}
void cleanDirectories() {
	uintmax_t n = fs::remove_all("images/compressed");
	n = fs::remove_all("images/resized");
	fs::create_directories("images/resized");
	fs::create_directories("images/compressed");
	
	for (dimension resolution : resolutions) {
		fs::create_directories("images/resized/" + resolution.name);
		fs::create_directories("images/resized/" + resolution.name + "/filters");
	}
}

// Covariance
double covariance(Mat & m1, Mat & m2, int i, int j, int block_height, int block_width)
{
	Mat m3 = Mat::zeros(block_height, block_width, m1.depth()); // Create 0 filled matrix 
	Mat m1_tmp = m1(Range(i, i + block_height), Range(j, j + block_width)); // Create temporary matrix (Range is used to generate the rows)
	Mat m2_tmp = m2(Range(i, i + block_height), Range(j, j + block_width));

	multiply(m1_tmp, m2_tmp, m3);

	double avg_co = mean(m3)[0]; // E(XY) medium point
	double avg_c = mean(m1_tmp)[0]; // E(X)
	double avg_o = mean(m2_tmp)[0]; // E(Y)

	double cov = avg_co - avg_o * avg_c; // E(XY) - E(X)E(Y)

	return cov;
}
// Luminance
double luminance(double avg_o, double avg_c) {
	return (2 * avg_o * avg_c + C1) / (pow(avg_o, 2) + pow(avg_c, 2) + C1);
}

// Contrast
double contrast(double sigma_o, double sigma_c) {
	return ((2 * sigma_o*sigma_c) + C2) / ((pow(sigma_o, 2) + pow(sigma_c, 2) + C2));
}

// Structure
double structure(double sigma_c, double sigma_o, double sigma_co) {
	return ((sigma_co + C3) / (sigma_o*sigma_c + C3));
}

// Variance
double variance(Mat & m, int i, int j, int block_height, int block_width)
{
	double var = 0;

	Mat m_tmp = m(Range(i, i + block_height), Range(j, j + block_width)); // Create temporary matrix (Range is used to generate the rows)
	Mat m_squared(block_height, block_width, CV_64F); // Create the matrix to scan

	multiply(m_tmp, m_tmp, m_squared);

	double avg = mean(m_tmp)[0]; 	// E(x) (mean calculate medium point)
	double avg_2 = mean(m_squared)[0]; 	// E(x�) 
	var = sqrt(avg_2 - avg * avg);
	return var;
}

vector<double> getValues(Mat img_src[3], Mat img_compressed[3], int block_height, int block_width, int answer)
{
	int nbBlockPerHeight = img_src[0].rows / block_height;
	int nbBlockPerWidth = img_src[0].cols / block_width;
	vector<double> values;

	double luminance_a;
	double luminance_b;
	double contrast_L;
	double structure_L;
	double ssim = 0;

	// Foreach block in the images
	for (int k = 0; k < nbBlockPerHeight; k++)
	{
		for (int l = 0; l < nbBlockPerWidth; l++)
		{
			int m = k * block_height;
			int n = l * block_width;
			// Avg values for b-channel
			double avg_ob = mean(img_src[0](Range(k, k + block_height), Range(l, l + block_width)))[0];
			double avg_cb = mean(img_compressed[0](Range(k, k + block_height), Range(l, l + block_width)))[0];
			values.push_back(avg_ob);
			values.push_back(avg_ob);

			// Avg values for a-channel
			double avg_oa = mean(img_src[1](Range(k, k + block_height), Range(l, l + block_width)))[0];
			double avg_ca = mean(img_compressed[1](Range(k, k + block_height), Range(l, l + block_width)))[0];
			values.push_back(avg_oa);
			values.push_back(avg_oa);

			// Avg values for l-channel
			double avg_oL = mean(img_src[2](Range(k, k + block_height), Range(l, l + block_width)))[0];
			double avg_cL = mean(img_compressed[2](Range(k, k + block_height), Range(l, l + block_width)))[0];
			values.push_back(avg_oL);
			values.push_back(avg_cL);

			// Sigma values for b-channel
			double sigma_oa = variance(img_src[2], m, n, block_height, block_width);
			double sigma_ca = variance(img_compressed[2], m, n, block_height, block_width);
			values.push_back(sigma_oa);
			values.push_back(sigma_ca);
	
			// Sigma values for a-channel
			double sigma_ob = variance(img_src[1], m, n, block_height, block_width);
			double sigma_cb = variance(img_compressed[1], m, n, block_height, block_width);
			values.push_back(sigma_ob);
			values.push_back(sigma_cb);

			// Sigma values for L-channel
			double sigma_oL = variance(img_src[0], m, n, block_height, block_width);
			double sigma_cL = variance(img_compressed[0], m, n, block_height, block_width);
			values.push_back(sigma_oL);
			values.push_back(sigma_cL);

			// SSIM
			if (answer == 1) {
				double avg_c = (avg_ca + avg_cb + avg_cL) / 3;
				double avg_o = (avg_oa + avg_ob + avg_oL) / 3;
				double sigma_c = (sigma_ca + sigma_cb + sigma_cL) / 3;
				double sigma_o = (sigma_oa + sigma_ob + sigma_oL) / 3;
				double sigma_co = covariance(img_src[0], img_compressed[0], m, n, block_height, block_width);
				luminance_a = luminance(avg_o, avg_c);
				luminance_b = luminance(avg_o, avg_c);
				contrast_L = contrast(sigma_o, sigma_c);
				structure_L = structure(sigma_o, sigma_c, sigma_co);
				ssim += (luminance_a + luminance_b + contrast_L + structure_L)/4;
			}

		}
	}
	ssim /= nbBlockPerHeight * nbBlockPerWidth;
	values.push_back(ssim);
	return values;
}

void getFeatures(int answer) {
	
	ofstream file;
	file.open("D:/dataset.txt");

	Mat originalImage, compressedImage;

	for (dimension resolution : resolutions) {
		string defaultSettings;
		string pathMaster = "D:/dataset/resized/";
		string pathFilters = "/filters";
		file << "\"Res\" " << "\"X\" \"Y\" ";
		for(int i=1;i<=9;i++){
			string j = to_string(i);
			file << "\"Mx"+j+"(L)\" \"My" + j + "(L)\" \"Sx" + j + "(L)\" \"Sy" + j + "(L)\" \"Mx" + j + "(a)\" \"My" + j + "(a)\" \"Sx" + j + "(a)\" \"Sy" + j + "(a)\" \"Mx" + j + "(b)\" \"My" + j + "(b)\" \"Sx" + j + "(b)\" \"Sy" + j + "(b)\" ";	
			if (i == 9 && answer == 1)
				file << " \"SSIM\"";
			if (i == 9) file << "; ..." << endl;
		
		}
		

		for (dimension resolution : resolutions) {
			
			for (auto& p : fs::directory_iterator(pathMaster+resolution.name)) { // Scorre tutti i Master
				
				string pathM = p.path().u8string();
				string masterName = explode((explode(explode(pathM, '/').back(), '.')[0]), '\\').back();
				string masterNumber = explode(masterName, '_')[0];
				
				Mat X = imread(pathM, CV_LOAD_IMAGE_UNCHANGED);
				cout << pathM << endl;
				if (X.empty()) continue;
				Mat X_Lab;
				cvtColor(X, X_Lab, COLOR_BGR2Lab);
				
				Mat X_splitted[3];
				split(X, X_splitted);


				Mat* X_normalized = normalizeLabValues(X_splitted);

				string pathFiltersLocal = pathMaster + resolution.name + pathFilters;
				
				for (auto& p : fs::directory_iterator(pathFiltersLocal)) {
					string pathF = p.path().u8string();
					string filterName = explode((explode(explode(pathF, '/').back(), '.')[0]),'\\').back();
					string filterNumber = explode(filterName, '_')[0];

					if (!masterNumber.compare(filterNumber)) {
						cout << "GENERATING FEATURES(" + resolution.name + ") FOR: " + masterName + "-" + filterName << endl;

						Mat Y = imread(pathF, CV_LOAD_IMAGE_UNCHANGED);

						Mat Y_Lab;
						cvtColor(Y, Y_Lab, COLOR_BGR2Lab);

						Mat Y_splitted[3];
						split(Y, Y_splitted);

						Mat* Y_normalized = normalizeLabValues(Y_splitted);
				
						
						vector<double> values = getValues(X_normalized, Y_normalized, resolution.height/resolution.blockheight, resolution.width/resolution.blockwidth, answer);
						file << "\"" << resolution.name << "\"" << " ";
						file << "\"" << masterName << "\"" << " ";
						file << "\""  << filterName << "\"" << " ";
						for (int j = 0; j < values.size(); j++) {
							if (j == values.size() - 1) {
								file << values[j] << "; ..." << endl;
							} else {
								file << values[j] << " ";
							}
						}
					}
				}
			}	
		}
	}
	
}

int main()
{	
	fillResolutions();
	while (1) {
		int answer;
		cout << "What do you want to do? Generate(1)/Convert to Lab(2)/Get features(3)/Clean(4)/Exit(5)" << endl;
		cin >> answer;

		if (answer == 1) {
			int colorSpace;
			cout << "Are your image CIELAB(1) or RGB(2)?";
			cin >> colorSpace;
			lowerQuality(colorSpace);
			cout << "Images generated successfully" << endl;
		}
		else if (answer == 2){
			convertToLab();
			cout << "Images converted to Lab" << endl;
		
		}else if (answer == 3) {
			int ssim;
			cout << "Do you want calculate the SSIM too? Yes(1)/No(2)" << endl;
			cin >> ssim;
			getFeatures(ssim);
			cout << "Features generated successfully" << endl;
		}
		else if (answer == 4) {
			cleanDirectories();
			cout << "Directory cleaned" << endl;
		}else break;
	}
	return 0;
}
