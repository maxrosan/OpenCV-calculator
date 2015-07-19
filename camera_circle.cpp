
#include <iostream>
#include <cstdlib>
#include <memory.h>
#include <fftw3.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <math.h>
#include <assert.h>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <pthread.h>

cv::Mat inputImage, originalImage;
cv::Mat accCenter;
std::vector<cv::Mat> accs;
std::vector<int> radius;
pthread_mutex_t mutex;


struct Button {

	int x, y;
	int r;
	std::string label;

	Button() {
	}

	Button (int x, int y, int r, std::string label) {
		this->x = x;
		this->y = y;
		this->r = r;
		this->label = label;
	}		

	Button (const Button &btn) {
		x = btn.x;
		y = btn.y;
		r = btn.r;
		label = btn.label;
	}	

	Button operator=(const Button &btn) {
		x = btn.x;
		y = btn.y;
		r = btn.r;
		label = btn.label;

		return *this;
	}

};

std::string expression = std::string("0");
cv::Point positionText;
std::pair<cv::Point, cv::Point> rectCalculator;
std::vector<Button> buttons;
cv::Point cursorPosition = cv::Point(0, 0);

std::string lastButton = std::string("#");
time_t firstEventTime = 0;

void addButtons() {

	positionText = cv::Point(60, 220);

	rectCalculator = std::make_pair(cv::Point(60, 240), cv::Point(360, 480));

	buttons.push_back(Button(100, 280, 25, "7"));
	buttons.push_back(Button(155, 280, 25, "8"));
	buttons.push_back(Button(210, 280, 25, "9"));
	buttons.push_back(Button(265, 280, 25, "/"));
	buttons.push_back(Button(320, 280, 25, "D"));

	buttons.push_back(Button(100, 335, 25, "4"));
	buttons.push_back(Button(155, 335, 25, "5"));
	buttons.push_back(Button(210, 335, 25, "6"));	
	buttons.push_back(Button(265, 335, 25, "*"));
	buttons.push_back(Button(320, 335, 25, "CE"));	

	buttons.push_back(Button(100, 390, 25, "1"));
	buttons.push_back(Button(155, 390, 25, "2"));
	buttons.push_back(Button(210, 390, 25, "3"));	
	buttons.push_back(Button(265, 390, 25, "-"));
	buttons.push_back(Button(320, 390, 25, "C"));		

	buttons.push_back(Button(100, 445, 25, "0"));
	buttons.push_back(Button(155, 445, 25, ","));
	buttons.push_back(Button(210, 445, 25, "="));	
	buttons.push_back(Button(265, 445, 25, "+"));
	buttons.push_back(Button(320, 445, 25, "Q"));		

}

std::string event;

void renderButtons() {

	int i;
	cv::Scalar color;
	double dx, dy, d;

	event = std::string("@");

	cv::rectangle(originalImage, 
		rectCalculator.first, 
		rectCalculator.second, 
		cv::Scalar(0, 255, 0));

	cv::putText(originalImage, 
		expression, 
		positionText, 
		cv::FONT_HERSHEY_SIMPLEX, 1, 
		cv::Scalar(255, 0, 0), 2);	

	for (i = 0; i < buttons.size(); i++) {

		color = cv::Scalar(0, 255, 0);

		dx = buttons[i].x - cursorPosition.x;
		dy = buttons[i].y - cursorPosition.y;

		if (sqrt(dx*dx + dy*dy) < buttons[i].r) {

			color = cv::Scalar(255, 255, 0);

			if (lastButton == buttons[i].label) {
				if (time(NULL) - firstEventTime > 2) {
					firstEventTime = time(NULL);
					color = cv::Scalar(0, 255, 255);
					event = buttons[i].label;
				}
			} else {
				lastButton = buttons[i].label;
				firstEventTime = time(NULL);
			}

		}

		cv::circle(originalImage, 
			cv::Point(buttons[i].x, buttons[i].y), buttons[i].r, color, 2);

		cv::putText(originalImage, 
			buttons[i].label, 
			cv::Point(buttons[i].x - 10, buttons[i].y + 10), 
			cv::FONT_HERSHEY_SIMPLEX, 0.5, 
			color, 1);
	}

}

void *findCircle(void *arg) {

	std::pair<int, int> *idx = (std::pair<int, int>*) arg;
	int a, b, r, d;

	for (b = rectCalculator.first.y; b <= rectCalculator.second.y; b++) {
		for (a = rectCalculator.first.x; a <= rectCalculator.second.x; a++) {

			if (inputImage.at<uchar>(b, a) == 0) {
				continue;
			}

			for (r = idx->first; r <= idx->second; r++) {

				int Rv = radius[r];

				for (d = 0; d <= 360; d += 10) {

					int x = (int) round(a - Rv * cos(d * 0.017453));
					int y = (int) round(b - Rv * sin(d * 0.017453));

					if ( x >= 0 && y >= 0 
						&& x < inputImage.size().width && y < inputImage.size().height) {

						accs[r].at<int>(y, x)++;

						if (accs[r].at<int>(y, x) > Rv / 4) {
							pthread_mutex_lock(&mutex);
							accCenter.at<int>(y, x)++;
							pthread_mutex_unlock(&mutex);
						}

					}
				}
			}
		}
	}

	free(idx);

	return NULL;
}

bool invalidExpr;
int idxParser;

double parseFactor() {

	double result = 0, afterPoint = 10.;
	bool pointFound = false, neg = false;

	if (expression[idxParser] == '-') {
		neg = true;
	}

	while (idxParser < expression.length()) {
		if (expression[idxParser] == ',' && !pointFound) {
			pointFound = true;
		} else if (expression[idxParser] >= '0' && expression[idxParser] <= '9') {
			if (!pointFound) {
				result += result * 10 + (expression[idxParser] - '0');
			} else {
				result += (expression[idxParser] - '0') / afterPoint;
				afterPoint *= 10.;
			}
		} else {
			break;
		}
		idxParser++;
	}

	if (neg) result *= -1.;

	return result;
}

double parseTerm() {

	double result;
	result = parseFactor();

	while (idxParser < expression.length() && !invalidExpr) {
		if (expression[idxParser] == '/') {
			idxParser++;
			result /= parseFactor();
		} else if (expression[idxParser] == '*') {
			idxParser++;
			result *= parseFactor();
		} else {
			break;
		}
	}

	return result;
}

double parseExpr() {

	double term;
	invalidExpr = false;
	idxParser = 0;

	term = parseTerm();

	while (idxParser < expression.length() && !invalidExpr) {
		if (expression[idxParser] == '+') {
			idxParser++;
			term += parseTerm();
		} else if (expression[idxParser] == '-') {
			idxParser++;
			term -= parseTerm();
		} else {
			std::cerr << "Expected + or - => " <<  expression[idxParser] << std::endl;
			invalidExpr = true;
			break;
		}
	}

	return term;

}

void buildExpression() {

	if (event[0] >= '0' && event[0] <= '9' ||
		event[0] == '+' ||
		event[0] == '-' ||
		event[0] == ',' ||
		event[0] == '/' ||
		event[0] == '*'		
		) {

		if (expression == std::string("0")) {
			expression = std::string("");
		}

		expression += std::string(event);

	} else if (event[0] == '=') {
		char strBuf[200];
		double val;

		val = parseExpr();

		if (!invalidExpr) {
			sprintf(strBuf, "%.2f", val);
			expression = std::string(strBuf);
		} else {
			std::cerr << "Invalid expression!" << std::endl;
			expression = std::string("0");
		}
	} else if (event == std::string("C") || event == std::string("CE")) {
		expression = std::string("0");
	} else if (event == std::string("D")) {
		expression = expression.substr(0, expression.length() - 1);
	} else if (event == std::string("Q")) {
		exit(EXIT_SUCCESS);
	}

}

int main(int argc, char **argv) {

	cv::VideoCapture cap (0);

	int a, b, r, d;
	int nThreads = 8;
	pthread_t threads[nThreads];
	int delta;

	for (a = 30; a <= 100; a++) {
		radius.push_back(a);
	}

	delta = radius.size() / nThreads;

	addButtons();

	pthread_mutex_init(&mutex, NULL);

	while (cap.read(inputImage)) {

		cv::flip(inputImage, inputImage, 1);
		originalImage = cv::Mat(inputImage);
		accCenter = cv::Mat(inputImage.size().height, inputImage.size().width, 
				CV_32S, cv::Scalar(0));

		cv::cvtColor(inputImage, inputImage, cv::COLOR_BGR2GRAY);
		cv::blur(inputImage, inputImage, cv::Size(8,8));
		cv::Canny(inputImage, inputImage, 30, 50);		

		accs.clear();

		for (r = 0; r < radius.size(); r++) {
			accs.push_back(cv::Mat(inputImage.size().height, inputImage.size().width, 
				CV_32S, cv::Scalar(0)));
		}

		for (a = 0; a < nThreads; a++) {
			std::pair<int, int>* idx = new std::pair<int, int>(a * delta, (a + 1) * delta - 1);
			pthread_create(&threads[a], NULL, findCircle, (void*) idx);
		}

		if (nThreads * delta < radius.size()) {
			std::pair<int, int>* idx = new std::pair<int, int>(nThreads * delta, radius.size() - 1);
			(void) findCircle(idx);
		}

		for (a = 0; a < nThreads; a++) {
			pthread_join(threads[a], NULL);
		}


		cursorPosition = cv::Point(0, 0);

		for (b = 0; b < inputImage.size().height; b++) {
			for (a = 0; a < inputImage.size().width; a++) {
				if (accCenter.at<int>(b, a) > 10) {
					cv::circle(originalImage, cv::Point(a, b), 10, cv::Scalar(255, 0, 0), 2);
					cursorPosition = cv::Point(a, b);
				}
			}
		}

		renderButtons();
		buildExpression();

		//cv::imshow("frame-input-R", accCenter);
		cv::imshow("frame-input", originalImage);

		if ((cv::waitKey(1) & 0xff) == 'q') break;

	}

	return EXIT_SUCCESS;
}