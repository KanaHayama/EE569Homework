/************************************
 *  Name: Zongjian Li               *
 *  USC ID: 6503378943              *
 *  USC Email: zongjian@usc.edu     *
 *  Submission Date: 3rd,Mar 2019   *
 ************************************/

 /*=================================
 |                                 |
 |              util               |
 |                                 |
 =================================*/

 //#define  _CRT_SECURE_NO_WARNINGS 
#define _USE_MATH_DEFINES

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <functional>

using namespace std;

const char * ArgumentOutOfRangeException = "Argument out of range";

const char * ArgumentException = "Wrong argument";

const char * InvalidOperationException = "Invalid operation";

const char * FailedToOpenFileException = "Failed to open file";

const char * DEFAULT_OUTPUT_FILENAME = "output.raw";

const int UNSIGNED_CHAR_MAX_VALUE = 0xFF;

const int GRAY_CHANNELS = 1;

const int COLOR_CHANNELS = 3;

enum class PaddingType {
	Zero,
	Reflect,
};

template <typename T>
class Image {
	private:
	

	protected:
	int Height;
	int Width;
	int Channel;
	vector<T> Data;
	PaddingType Padding;

	int getSize() const {
		return Height * Width * Channel;
	}

	int getMemSize() const {
		return sizeof(unsigned char) * getSize();
	}

	static int reflect(const int size, const int index) {
		if (index <= -size || index >= size * 2 - 1) {
			throw ArgumentOutOfRangeException;
		}
		if (index < 0) {
			return -index;
		} else if (index >= size) {
			return 2 * size - index - 2;
		} else {
			return index;
		}
	}

	int index(const int row, const int col, const int ch) const {
		return Channel * (reflect(Height, row) * Width + reflect(Width, col)) + ch;
	}

	double LinearInterpolation(const T a, const T b, const double fa) const {
		if (fa < 0 || fa > 1) {
			throw ArgumentOutOfRangeException;
		}
		return a * fa + b * (1 - fa);
	}

	public:
	Image() : Height(0), Width(0), Channel(1), Padding(PaddingType::Reflect) {
		Data = vector<T>();
	}

	Image(const int height, const int width, const int channel, const PaddingType padding = PaddingType::Reflect) : Height(height), Width(width), Channel(channel), Padding(padding) {
		Data = vector<T>(getSize());
	}

	Image(const int height, const int width, const int channel, const T & fill) : Image(height, width, channel) {
		auto size = getSize();
		for (int i = 0; i < size; i++) {
			Data[i] = fill;
		}
	}

	Image(const int height, const int width, const int channel, const T data[]) : Image(height, width, channel) {
		auto size = getSize();
		for (int i = 0; i < size; i++) {
			Data[i] = data[i];
		}
	}

	Image(const int height, const int width, const int channel, string filename) : Image(height, width, channel) {
		auto s = ifstream(filename, ifstream::binary);
		if (!s) {
			throw FailedToOpenFileException;
		}
		s.read((char *)&(Data[0]), getMemSize());
		s.close();
	}

	Image(const vector<Image> & channels) {
		if (channels.empty()) {
			throw ArgumentOutOfRangeException;
		}
		auto sample = channels.front();
		for (auto & ch : channels) {
			if (ch.Channel != 1) {
				throw ArgumentException;
			}
			if (ch.Width != sample.Width || ch.Height != sample.Height) {
				throw ArgumentException;
			}
		}
		Height = sample.Height;
		Width = sample.Width;
		Channel = channels.size();
		Data = vector<T>(getSize());
		Padding = PaddingType::Reflect;
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j++) {
				for (int ch = 0; ch < Channel; ch++) {
					setValue(i, j, ch, channels[ch].getValue(i, j, 0));
				}
			}
		}
	}

	int getHeight() const {
		return Height;
	}

	int getWidth() const {
		return Width;
	}

	int getChannel() const {
		return Channel;
	}

	T getValue(const int row, const int col, const int ch, const bool enablePadding = true) const {
		if (row < 0 || row >= Height || col < 0 || col >= Width || ch < 0 || ch >= Channel) {
			if (!enablePadding) {
				throw ArgumentOutOfRangeException;
			} else if (Padding == PaddingType::Zero) {
				return 0;
			}
		}
		return Data[index(row, col, ch)];
	}

	double getLinearInterplatedValue(const double row, const double col, const int ch, const bool enablePadding = true) const {
		if (row < 0 || row >= Height || col < 0 || col >= Width || ch < 0 || ch >= Channel) {
			if (!enablePadding) {
				throw ArgumentOutOfRangeException;
			} else if (Padding == PaddingType::Zero) {
				return 0;
			}
		}
		// neighbors
		auto rowLower = floor(row);
		auto rowHigher = ceil(row);
		auto colLower = floor(col);
		auto colHigher = ceil(col);
		auto valueLeftTop = getValue(rowLower, colLower, 0);
		auto valueLeftBottom = getValue(rowHigher, colLower, 0);
		auto valueRightTop = getValue(rowLower, colHigher, 0);
		auto valueRightBottom = getValue(rowHigher, colHigher, 0);
		//row direction interpolation
		auto lin1 = LinearInterpolation(valueLeftTop, valueRightTop, 1 - (col - colLower));
		auto lin2 = LinearInterpolation(valueLeftBottom, valueRightBottom, 1 - (col - colLower));
		//col direction interpolation
		auto pixel = LinearInterpolation(lin1, lin2, 1 - (row - rowLower));
		return pixel;
	}

	void setValue(const int row, const int col, const int ch, const T & value) {
		if (row < 0 || row >= Height || col < 0 || col >= Width || ch < 0 || ch >= Channel) {
			throw ArgumentOutOfRangeException;
		}
		Data[index(row, col, ch)] = value;
	}

	PaddingType getPadding() const {
		return Padding;
	}

	void setPadding(const PaddingType padding) {
		Padding = padding;
	}

	Image clip(const T lower, const T upper) const {
		auto result = Image(Height, Width, Channel);
		for (int i = 0; i < getSize(); i++) {
			result.Data[i] = min(upper, max(lower, Data[i]));
		}
	}

	Image round() const {
		auto result = Image(Height, Width, Channel);
		for (int i = 0; i < getSize(); i++) {
			result.Data[i] = int(Data[i] + 0.5);
		}
	}

	Image comp(const T upper = 0) const {
		auto result = Image(Height, Width, Channel);
		for (int i = 0; i < getSize(); i++) {
			result.Data[i] = upper - Data[i];
		}
		return result;
	}

	vector<Image> split() const {
		auto result = vector<Image>(Channel);
		for (int ch = 0; ch < Channel; ch++) {
			result[ch] = Image(Height, Width, 1);
			for (int i = 0; i < Height; i++) {
				for (int j = 0; j < Width; j++) {
					result[ch].setValue(i, j, 0, getValue(i, j, ch));
				}
			}
		}
		return result;
	}

	void writeToFile(const string & filename) const {
		auto s = ofstream(filename, ofstream::binary);
		if (!s) {
			throw FailedToOpenFileException;
		}
		s.write((char *)&Data[0], getMemSize());
		s.close();
	}

	~Image() {

	}
};

const float THRESHOLD = 0.5 * UNSIGNED_CHAR_MAX_VALUE;

/*=================================
|                                 |
|                a)               |
|                                 |
=================================*/

const int PIECES_HEIGHT = 256;
const int PIECES_WIDTH = 256;

const int DEFAULT_HEIGHT = 512;
const int DEFAULT_WIDTH = 512;

const char * PIECE_1_FILENAME = "lighthouse1.raw";
const char * PIECE_2_FILENAME = "lighthouse2.raw";
const char * PIECE_3_FILENAME = "lighthouse3.raw";
const char * LIGHTHOUSE_FILENAME = "lighthouse.raw";

template<typename T>
struct Coordinate {
	T Row;
	T Col;

	Coordinate(const int row = 0, const int col = 0) : Row(row), Col(col) {}

	bool operator == (const Coordinate & other) const {
		return Row == other.Row && Col == other.Col;
	}
};

template<typename T>
struct Corners {
	Coordinate<T> LeftTop;
	Coordinate<T> RightTop;
	Coordinate<T> RightBottom;
	Coordinate<T> LeftBottom;
};

vector<Corners<int>> FindHoles(const Image<unsigned char> & input) {
	if (input.getChannel() != GRAY_CHANNELS) {
		throw ArgumentException;
	}
	auto result = vector<Corners<int>>();		
	//column major!
	auto corners = Corners<int>();
	for (int j = 0; j < input.getWidth(); j++) {
		for (int i = 0; i < input.getHeight(); i++) {
			auto center = input.getValue(i, j, 0);
			auto north = input.getValue(i - 1, j, 0);
			auto south = input.getValue(i + 1, j, 0);
			auto west = input.getValue(i, j - 1, 0);
			auto east = input.getValue(i, j + 1, 0);
			if (center == UNSIGNED_CHAR_MAX_VALUE && north != UNSIGNED_CHAR_MAX_VALUE && south == UNSIGNED_CHAR_MAX_VALUE && west != UNSIGNED_CHAR_MAX_VALUE && east == UNSIGNED_CHAR_MAX_VALUE) {
				corners.LeftTop.Row = i;
				corners.LeftTop.Col = j;
				auto jj = j;
				while (input.getValue(i, jj + 1, 0) == UNSIGNED_CHAR_MAX_VALUE) {
					jj++;
				}
				corners.RightTop.Row = i;
				corners.RightTop.Col = jj;
				auto ii = i;
				while (input.getValue(ii + 1, j, 0) == UNSIGNED_CHAR_MAX_VALUE) {
					ii++;
				}
				corners.LeftBottom.Row = ii;
				corners.LeftBottom.Col = j;
				corners.RightBottom.Row = ii;
				corners.RightBottom.Col = jj;
				auto pass = true;
				for (int iii = i; iii <= ii; iii++) {
					for (int jjj = j; jjj <= jj; jjj++) {
						if (input.getValue(iii, jjj, 0) != UNSIGNED_CHAR_MAX_VALUE) {
							pass = false;
							goto Add;
						}
					}
				}
			Add:
				if (pass && corners.RightTop.Col > corners.LeftTop.Col && corners.LeftBottom.Row > corners.LeftTop.Row) {
					result.push_back(corners);
				}				
			}
		}
	}
	//check
	if (result.size() != 3) {
		throw ArgumentException;
	}
	return result;
}

template<typename T>
struct Range {
	Coordinate<T> RowMin;
	Coordinate<T> RowMax;
	Coordinate<T> ColMin;
	Coordinate<T> ColMax;
};

double CalcRangeSlopeError(const Range<int> & range) {
	auto lt = atan2(range.ColMin.Col - range.RowMin.Col, range.ColMin.Row - range.RowMin.Row);
	auto rb = atan2(range.RowMax.Col - range.ColMax.Col, range.RowMax.Row - range.ColMax.Row);
	auto diff1 = abs(lt - rb);
	auto lb = atan2(range.RowMax.Col - range.ColMin.Col, range.RowMax.Row - range.ColMin.Row);
	auto rt = atan2(range.ColMax.Col - range.RowMin.Col, range.ColMax.Row - range.RowMin.Row);
	auto diff2 = abs(lb - rt);
	auto result = diff1 + diff2;
	return result;
}

Range<int> FindRange(const Image<unsigned char> & input, const int shrink) {
	if (input.getChannel() != GRAY_CHANNELS) {
		throw ArgumentException;
	}
	auto found = Range<int>();
	//RowMin
FindRowMin:
	for (int i = 0; i < input.getHeight(); i++) {
		auto min = UNSIGNED_CHAR_MAX_VALUE;
		found.RowMin.Row = i + 1 * shrink;
		for (int j = 0; j < input.getWidth(); j++) {
			auto v = input.getValue(i, j, 0);
			if (v < min) {
				min = v;
				found.RowMin.Col = j;
			}
		}
		if (min < UNSIGNED_CHAR_MAX_VALUE) {
			goto FindRowMax;
		}						
	}
	//RowMax
FindRowMax:
	for (int i = input.getHeight() - 1; i >= 0; i--) {
		auto min = UNSIGNED_CHAR_MAX_VALUE;
		found.RowMax.Row = i + -1 * shrink;
		for (int j = 0; j < input.getWidth(); j++) {
			auto v = input.getValue(i, j, 0);
			if (v < min) {
				min = v;
				found.RowMax.Col = j;				
			}
		}
		if (min < UNSIGNED_CHAR_MAX_VALUE) {
			goto FindColMin;
		}
	}
	//ColMin
FindColMin:
	for (int j = 0; j < input.getWidth(); j++) {
		auto min = UNSIGNED_CHAR_MAX_VALUE;
		found.ColMin.Col = j + 1 * shrink;
		for (int i = 0; i < input.getHeight(); i++) {		
			auto v = input.getValue(i, j, 0);
			if (v < min) {
				min = v;
				found.ColMin.Row = i;				
			}
		}
		if (min < UNSIGNED_CHAR_MAX_VALUE) {
			goto FindColMax;
		}
	}

	//ColMax
FindColMax:
	for (int j = input.getWidth() - 1; j >- 0; j--) {
		auto min = UNSIGNED_CHAR_MAX_VALUE;
		found.ColMax.Col = j + -1 * shrink;
		for (int i = 0; i < input.getHeight(); i++) {
			auto v = input.getValue(i, j, 0);
			if (v < min) {
				min = v;
				found.ColMax.Row = i;
			}
		}
		if (min < UNSIGNED_CHAR_MAX_VALUE) {
			goto Check;
		}
	}
	//Check
Check:
	if (found.RowMin.Row >= found.RowMax.Row || found.ColMin.Col >= found.ColMax.Col) {
		throw ArgumentException;
	}
	// Adjust
	auto result = Range<int>();
	auto minError = numeric_limits<double>::max();
	for (int trial = 0; trial <= (1 << 4) - 1; trial++) {
		auto temp = found;
		for (int side = 0; side < 4; side++) {
			auto shrinkOneMorePixel = bool(trial & (1 << side));
			switch (side) {
				case 0:
					temp.RowMin.Row++;
					break;
				case 1:
					temp.RowMax.Row--;
					break;
				case 2:
					temp.ColMin.Col++;
					break;
				case 3:
					temp.ColMax.Col--;
					break;
			}
			auto error = CalcRangeSlopeError(temp);
			if (error < minError) {
				minError = error;
				result = temp;
			}
		}
	}
	return result;
}

double FindRawRotation(const Range<int> & input) {
	auto angle = atan2(input.RowMax.Col - input.ColMin.Col, input.RowMax.Row - input.ColMin.Row);//conside x-y aixes directions
	return angle;
}


Coordinate<double> FindScaling(const Corners<int> & source, const Range<int> & destination, const double rotate) {
	auto sourceLen1 = source.LeftBottom.Row - source.LeftTop.Row;
	auto sourceLen2 = source.RightTop.Col - source.LeftTop.Col;
	auto destLen1 = sqrt(pow(destination.RowMax.Row - destination.ColMin.Row, 2) + pow(destination.RowMax.Col - destination.ColMin.Col, 2));
	auto destLen2 = sqrt(pow(destination.ColMin.Row - destination.RowMin.Row, 2) + pow(destination.RowMin.Col - destination.ColMin.Col, 2));	
	auto result = Coordinate<double>();
	if ((0 * M_PI <= rotate && rotate < 0.5 * M_PI) || (1 * M_PI <= rotate && rotate < 1.5 * M_PI)) {
		result.Row = destLen1 / sourceLen1;
		result.Col = destLen2 / sourceLen2;
	} else {
		result.Row = destLen2 / sourceLen1;
		result.Col = destLen1 / sourceLen2;
	}
	return result;
}

Coordinate<double> FindLastTranslation(const Corners<int> & source, const Range<int> & destination, const double rotate) {
	auto sourceCoord = source.LeftTop;
	auto destCoord = Coordinate<int>();
	if (0.0 * M_PI <= rotate && rotate < 0.5 * M_PI) {
		destCoord = destination.ColMin;
	} else if (0.5 * M_PI <= rotate && rotate < 1.0 * M_PI) {
		destCoord = destination.RowMax;
	} else if (1.0 * M_PI <= rotate && rotate < 1.5 * M_PI) {
		destCoord = destination.ColMax;
	} else if (1.5 * M_PI <= rotate && rotate < 2.0 * M_PI) {
		destCoord = destination.RowMin;
	} else {
		throw ArgumentOutOfRangeException;
	}
	auto result = Coordinate<double>();
	result.Row = destCoord.Row/* - sourceCoord.Row*/;
	result.Col = destCoord.Col/* - sourceCoord.Col*/;
	return result;
}

void MatrixMultiplyMatrix(const double a[3][3], const double b[3][3], double result[3][3]) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			result[i][j] = 0;
			for (int k = 0; k < 3; k++) {
				result[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

void MatrixMultiplyVector(const double a[3][3], const double b[3], double result[3]) {
	for (int i = 0; i < 3; i++) {
		result[i] = 0;
		for (int j = 0; j < 3; j++) {
			result[i] += a[i][j] * b[j];
		}
	}
}

unsigned char roundAndClip(const double x) {
	if (x > UNSIGNED_CHAR_MAX_VALUE) {
		return UNSIGNED_CHAR_MAX_VALUE;
	} else if (x < 0) {
		return 0;
	} else {
		return round(x);
	}
}

double BoundaryMatchError(const Image<unsigned char> & input, const Corners<int> & hole) {
	auto error = 0.0;
	auto count = 0;
	// North
	if (hole.LeftTop.Row > 0) {
		auto i = hole.LeftTop.Row;
		for (auto j = hole.LeftTop.Col; j != hole.RightTop.Col; j++) {
			error += abs(int(input.getValue(i + 1, j, 0)) - int(input.getValue(i - 1, j, 0)));
			count++;
		}
	}
	// South
	if (hole.LeftBottom.Row < input.getHeight() - 1) {
		auto i = hole.LeftBottom.Row;
		for (auto j = hole.LeftBottom.Col; j != hole.RightBottom.Col; j++) {
			error += abs(int(input.getValue(i - 1, j, 0)) - int(input.getValue(i + 1, j, 0)));
			count++;
		}
	}
	// West
	if (hole.LeftTop.Col > 0) {
		auto j = hole.LeftTop.Col;
		for (auto i = hole.LeftTop.Row; i != hole.LeftBottom.Row; i++) {
			error += abs(int(input.getValue(i, j + 1, 0)) - int(input.getValue(i, j - 1, 0)));
			count++;
		}
	}
	// East
	if (hole.RightTop.Col < input.getWidth() - 1) {
		auto j = hole.RightTop.Col;
		for (auto i = hole.RightTop.Row; i != hole.RightBottom.Row; i++) {
			error += abs(int(input.getValue(i, j - 1, 0)) - int(input.getValue(i, j + 1, 0)));
			count++;
		}
	}
	return error / count;
}

Image<unsigned char> GeometricTransformation(const Image<unsigned char> & input, const Corners<int> & hole, const Image<unsigned char> & piece, const double rotate, const Coordinate<double> & scale, const Coordinate<double> & translateLast) {
	// create matrices
	double matrix[3][3];
	{
		double translateFirstMatrix[3][3] = { 0 };
		translateFirstMatrix[0][0] = 1;
		translateFirstMatrix[0][2] = -hole.LeftTop.Row;
		translateFirstMatrix[1][1] = 1;
		translateFirstMatrix[1][2] = -hole.LeftTop.Col;
		translateFirstMatrix[2][2] = 1;
		double rotateMatrix[3][3] = { 0 };
		rotateMatrix[0][0] = cos(rotate);
		rotateMatrix[0][1] = -sin(rotate);
		rotateMatrix[1][0] = sin(rotate);
		rotateMatrix[1][1] = cos(rotate);
		rotateMatrix[2][2] = 1;
		double scaleMatrix[3][3] = { 0 };
		scaleMatrix[0][0] = scale.Row;
		scaleMatrix[1][1] = scale.Col;
		scaleMatrix[2][2] = 1;
		double translateLastMatrix[3][3] = { 0 };
		translateLastMatrix[0][0] = 1;
		translateLastMatrix[0][2] = translateLast.Row;
		translateLastMatrix[1][1] = 1;
		translateLastMatrix[1][2] = translateLast.Col;
		translateLastMatrix[2][2] = 1;
		// multiply matrices
		double tempMatrix1[3][3];
		MatrixMultiplyMatrix(translateLastMatrix, scaleMatrix, tempMatrix1);
		double tempMatrix2[3][3];
		MatrixMultiplyMatrix(tempMatrix1, rotateMatrix, tempMatrix2);
		MatrixMultiplyMatrix(tempMatrix2, translateFirstMatrix, matrix);
	}
	// mapping
	auto result = input;
	for (int i = hole.LeftTop.Row; i <= hole.LeftBottom.Row; i++) {
		for (int j = hole.LeftTop.Col; j <= hole.RightTop.Col; j++) {
			double source[3];
			source[0] = i;
			source[1] = j;
			source[2] = 1;
			double dest[3];
			MatrixMultiplyVector(matrix, source, dest);
			auto row = dest[0];
			auto col = dest[1];
			result.setValue(i, j, 0, roundAndClip(piece.getLinearInterplatedValue(row, col, 0)));
		}
	}
	return result;
}

Image<unsigned char> FillHole(const Image<unsigned char> & input, const Corners<int> & hole, const Image<unsigned char> & piece, const int shrink) {
	auto range = FindRange(piece, shrink);
	auto rawRotate = FindRawRotation(range);

	auto minError = numeric_limits<double>::max();
	auto result = Image<unsigned char>();
	for (double f = 0; f < 2; f += 0.5) {
		auto rotate = rawRotate + f * M_PI;
		auto scale = FindScaling(hole, range, rotate);
		auto translateLast = FindLastTranslation(hole, range, rotate);
		auto tempResult =  GeometricTransformation(input, hole, piece, rotate, scale, translateLast);
		auto error = BoundaryMatchError(tempResult, hole);
		if (error < minError) {
			minError = error;
			result = tempResult;
		}
	}
	return result;
}

Image<unsigned char> FillHoles(const string & path, const int shrink) {
	auto image = Image<unsigned char>(DEFAULT_HEIGHT, DEFAULT_WIDTH, GRAY_CHANNELS, path + "/" + LIGHTHOUSE_FILENAME);
	auto holes = FindHoles(image);

	auto hole1 = holes[0];
	auto piece1 = Image<unsigned char>(PIECES_HEIGHT, PIECES_WIDTH, GRAY_CHANNELS, path + "/" + PIECE_1_FILENAME);
	image = FillHole(image, hole1, piece1, shrink);
	
	auto hole2 = holes[1];
	auto piece2 = Image<unsigned char>(PIECES_HEIGHT, PIECES_WIDTH, GRAY_CHANNELS, path + "/" + PIECE_2_FILENAME);
	image = FillHole(image, hole2, piece2, shrink);

	auto hole3 = holes[2];
	auto piece3 = Image<unsigned char>(PIECES_HEIGHT, PIECES_WIDTH, GRAY_CHANNELS, path + "/" + PIECE_3_FILENAME);
	image = FillHole(image, hole3, piece3, shrink);
	
	return image;
}

/*=================================
|                                 |
|                b)               |
|                                 |
=================================*/

const int MATRIX_SIZE = 6;

double CalcDeterminant(const double input[MATRIX_SIZE][MATRIX_SIZE], const int n) {
	if (n == 1) {
		return input[0][0];
	}
	auto result = 0.0;
	double temp[MATRIX_SIZE][MATRIX_SIZE] = { 0 };
	for (int k = 0; k < n; k++) {
		for (int i = 0; i < n - 1; i++) {
			for (int j = 0; j < n - 1; j++) {
				temp[i][j] = input[i + 1][j >= k ? j + 1 : j];
			}
		}
		auto sub = CalcDeterminant(temp, n - 1);
		auto sign = k % 2 == 0 ? 1 : -1;
		result += sign * input[0][k] * sub;
	}
	return result;
}

void CalcAdjointMatrix(const double input[MATRIX_SIZE][MATRIX_SIZE], const int n, double result[MATRIX_SIZE][MATRIX_SIZE]) {
	if (n == 1) {
		result[0][0] = 1;
		return;
	}
	double temp[MATRIX_SIZE][MATRIX_SIZE];
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			for (int ii = 0; ii < n - 1; ii++) {
				for (int jj = 0; jj < n - 1; jj++) {
					temp[ii][jj] = input[ii >= i ? ii + 1 : ii][jj >= j ? jj + 1 : jj];
				}
			}
			auto sign = (i + j) % 2 == 0 ? 1 : -1;
			result[j][i] = sign * CalcDeterminant(temp, n - 1);
		}
	}
}

bool CalcInverse(const double input[MATRIX_SIZE][MATRIX_SIZE], double result[MATRIX_SIZE][MATRIX_SIZE]) {
	auto det = CalcDeterminant(input, MATRIX_SIZE);
	if (det == 0) {
		return false;
	}
	double temp[MATRIX_SIZE][MATRIX_SIZE];
	CalcAdjointMatrix(input, MATRIX_SIZE, temp);
	for (int i = 0; i < MATRIX_SIZE; i++) {
		for (int j = 0; j < MATRIX_SIZE; j++) {
			result[i][j] = temp[i][j] / det;
		}
	}
	return true;
}

void Make_6by6_XY_Matrix(const double coordinates[MATRIX_SIZE][2], double result[MATRIX_SIZE][MATRIX_SIZE]) {
	for (int j = 0; j < MATRIX_SIZE; j++) {
		result[0][j] = 1;
		result[1][j] = coordinates[j][0];
		result[2][j] = coordinates[j][1];
		result[3][j] = coordinates[j][0] * coordinates[j][0];
		result[4][j] = coordinates[j][0] * coordinates[j][1];
		result[5][j] = coordinates[j][1] * coordinates[j][1];
	}
}

void Make_2by6_UV_Matrix(const double coordinates[MATRIX_SIZE][2], double result[2][MATRIX_SIZE]) {
	for (int j = 0; j < MATRIX_SIZE; j++) {
		result[0][j] = coordinates[j][0];
		result[1][j] = coordinates[j][1];
	}
}

void Make_6by1_XY_Vector(const double row, const double col, double result[MATRIX_SIZE]) {
	result[0] = 1;
	result[1] = row;
	result[2] = col;
	result[3] = row * row;
	result[4] = row * col;
	result[5] = col * col;
}

///matrix multiplication
void CalcFactors(const double a[2][MATRIX_SIZE], const double b[MATRIX_SIZE][MATRIX_SIZE], double result[2][MATRIX_SIZE]) {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < MATRIX_SIZE; j++) {
			result[i][j] = 0;
			for (int k = 0; k < MATRIX_SIZE; k++) {
				result[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

///matrix multiplication
void CalcWarppedCoordinate(const double a[2][MATRIX_SIZE], const double b[MATRIX_SIZE], double result[2]) {
	for (int i = 0; i < 2; i++) {
		result[i] = 0;
		for (int j = 0; j < MATRIX_SIZE; j++) {
			result[i] += a[i][j] * b[j];
		}
	}
}

///Anchors in hat.raw, since I use reverse mapping, so it is called WARPPED
const double WARPPED_ANCHORS[4][MATRIX_SIZE][2]{ // {row, col}
	//North
	{
		{0.5, 0.5}, {0.0, 0.0}, {0.0, 1.0}, 
		{0.25, 0.25}, {0.25, 0.75}, 
		{0.0, 0.5},
	},
	//South
	{
		{0.5, 0.5}, {1.0, 0.0}, {1.0, 1.0},
		{0.75, 0.25}, {0.75, 0.75},
		{1.0, 0.5},
	},
	//West
	{
		{0.5, 0.5}, {0.0, 0.0}, {1.0, 0.0}, 
		{0.25, 0.25}, {0.75, 0.25},
		{0.5, 0.0},
	},
	//East
	{
		{0.5, 0.5}, {0.0, 1.0}, {1.0, 1.0},
		{0.25, 0.75}, {0.75, 0.75},
		{0.5, 1.0},
	},
};

const double UNWARPPED_ANCHORS[4][MATRIX_SIZE][2]{ // {row, col}
	//North
	{
		{0.5, 0.5}, {0.0, 0.0}, {0.0, 1.0}, 
		{0.25, 0.25}, {0.25, 0.75},
		{0.25, 0.5},
	},
	//South
	{
		{0.5, 0.5}, {1.0, 0.0}, {1.0, 1.0},
		{0.75, 0.25}, {0.75, 0.75},
		{0.75, 0.5},
	},
	//West
	{
		{0.5, 0.5}, {0.0, 0.0}, {1.0, 0.0},
		{0.25, 0.25}, {0.75, 0.25},
		{0.5, 0.25},
	},
	//East
	{
		{0.5, 0.5}, {0.0, 1.0}, {1.0, 1.0},
		{0.25, 0.75}, {0.75, 0.75},
		{0.5, 0.75},
	},
};

bool InTriangle(const double y, const double x, const double y1, const double x1, const double y2, const double x2, const double y3, const double x3) {
	auto den = ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	auto a = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / den;
	auto b = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / den;
	auto c = 1 - a - b;

	return 0 <= a && a <= 1 && 0 <= b && b <= 1 && 0 <= c && c <= 1;
}

Image<unsigned char> Warp(const Image<unsigned char> & input) {
	auto result = Image<unsigned char>(input.getHeight(), input.getWidth(), input.getChannel(), (unsigned char)0);
	for (int direction = 0; direction < 4; direction++) {
		// calc factors (inverse-mapping!)
		double factors[2][MATRIX_SIZE];
		{
			double uv_2by6[2][MATRIX_SIZE];
			Make_2by6_UV_Matrix(WARPPED_ANCHORS[direction], uv_2by6);
			double xy_6by6[MATRIX_SIZE][MATRIX_SIZE];
			Make_6by6_XY_Matrix(UNWARPPED_ANCHORS[direction], xy_6by6);
			double xy_6by6_inverse[MATRIX_SIZE][MATRIX_SIZE];
			auto succ = CalcInverse(xy_6by6, xy_6by6_inverse);
			if (!succ) {
				throw InvalidOperationException;
			}
			CalcFactors(uv_2by6, xy_6by6_inverse, factors);
		}
		//mapping
		auto vertScale = input.getHeight()/* - 1*/; // no -1, otherwise there will be holes that do not belong to any triangle!
		auto horiScale = input.getWidth()/* - 1*/; // no -1, otherwise there will be holes that do not belong to any triangle!
		for (int i = 0; i < input.getHeight(); i++) {
			auto row = double(i) / vertScale;
			for (int j = 0; j < input.getWidth(); j++) {				
				auto col = double(j) / horiScale;
				if (InTriangle(row, col, WARPPED_ANCHORS[direction][0][0], WARPPED_ANCHORS[direction][0][1], WARPPED_ANCHORS[direction][1][0], WARPPED_ANCHORS[direction][1][1], WARPPED_ANCHORS[direction][2][0], WARPPED_ANCHORS[direction][2][1])) {
					double xy_6by1[MATRIX_SIZE];
					Make_6by1_XY_Vector(row, col, xy_6by1);
					double mapping[2];
					CalcWarppedCoordinate(factors, xy_6by1, mapping);
					auto originalRow = mapping[0] * vertScale;
					auto originalCol = mapping[1] * horiScale;
					if (0 <= originalRow && originalRow <= input.getHeight() - 1 && 0 <= originalCol && originalCol <= input.getWidth() - 1) {
						result.setValue(i, j, 0, input.getLinearInterplatedValue(originalRow, originalCol, 0));
					}					
				}
			}
		}
	}
	return result;
}

/*=================================
|                                 |
|                c)               |
|                                 |
=================================*/

const double K1 = -0.3536;
const double K2 = 0.1730;
const double F = 600;

auto GenerateMapping(const int height, const int width) { // C++14
	auto rowCenter = (height - 1) * 0.5;
	auto colCenter = (width - 1) * 0.5;
	auto result = unordered_map<Coordinate<int>, Coordinate<int>, size_t(*)(const Coordinate<int>&)>(height * width, [](const Coordinate<int>& o) -> size_t { return hash<int>()(o.Row) + hash<int>()(o.Col); });
	for (int i = 0; i < height; i++) {
		auto row = (i - rowCenter) / F;
		for (int j = 0; j < width; j++) {			
			auto col = (j - colCenter) / F;
			auto rSqr = pow(row, 2) + pow(col, 2);
			auto factor = 1 + K1 * rSqr + K2 * pow(rSqr, 2);
			auto mappedRow = row * factor;
			auto mappedCol = col * factor;
			mappedRow = mappedRow * F + rowCenter;
			mappedCol = mappedCol * F + colCenter;
			auto mappedRowI = int(round(mappedRow));
			auto mappedColI = int(round(mappedCol));
			if (0 <= mappedRowI && mappedRowI < height && 0 <= mappedColI && mappedColI < width) {
				result[Coordinate<int>(mappedRowI, mappedColI)] = Coordinate<int>(i, j);
			}			
		}
	}
	return result;
}

Image<int> SetPixels(const Image<unsigned char> & input) {
	auto mapping = GenerateMapping(input.getHeight(), input.getWidth());
	auto inputBackup = input;
	auto result = Image<int>(input.getHeight(), input.getWidth(), input.getChannel(), (int)-1);
	for (int i = 0; i < input.getHeight(); i++) {
		for (int j = 0; j < input.getWidth(); j++) {
			auto pair = mapping.find(Coordinate<int>(i, j));
			if (pair == mapping.end()) {
				//cout << i << "\t" << j << endl;				
			} else {
				auto trueCoordinate = pair->second;
				result.setValue(trueCoordinate.Row, trueCoordinate.Col, 0, input.getValue(i, j, 0));// set distorted pixels to their real positon
				inputBackup.setValue(i, j, 0, 0);
			}
		}
	}
	inputBackup.writeToFile("UnmappedPixels.raw");
	return result;
}

Image<unsigned char> LensDistrotionCorrection(const Image<unsigned char> & input) {
	auto temp = SetPixels(input);
	//TODO: other operations

	//change value type
	auto result = Image<unsigned char>(input.getHeight(), input.getWidth(), input.getChannel(), (unsigned char)0);
	for (int i = 0; i < input.getHeight(); i++) {
		for (int j = 0; j < input.getWidth(); j++) {
			auto v = temp.getValue(i, j, 0);
			result.setValue(i, j, 0, roundAndClip(v));
		}
	}
	return result;

}

/*=================================
|                                 |
|              main               |
|                                 |
=================================*/

const char * OPTION_PROBLEM = "-p";
const char * OPTION_SHRINK = "-s";
const char * OPTION_OUTPUT = "-o";
const char * OPTION_HEIGHT = "-h";
const char * OPTION_WIDTH = "-w";
const char * OPTION_CHANNEL = "-c";

const char * PROBLEM_A = "a";
const char * PROBLEM_B = "b";
const char * PROBLEM_C = "c";

enum class Problem {
	A,
	B,
	C,
};

const char * WrongCommandException = "Wrong command";

Problem ParseFunction(const string & cmd) {
	if (cmd == PROBLEM_A) {
		return Problem::A;
	} else if (cmd == PROBLEM_B) {
		return Problem::B;
	} else if (cmd == PROBLEM_C) {
		return Problem::C;
	} else {
		throw WrongCommandException;
	}
}

const Problem DEFAULT_PROBLEM = Problem::A;
const int DEFAULT_SHRINK = 1;
const int DEFAULT_CHANNEL = GRAY_CHANNELS;

void PrintUsage() {
	cerr << "Usage:" << endl
		<< "\t" << "Problem1 [OPTION]... [INPUT FILE / DIR]" << endl
		<< endl
		<< "Intro:" << endl
		<< "\t" << "Geometric Modification." << endl
		<< "\t" << "For USC EE569 2019 spring home work 3 problem 1 by Zongjian Li." << endl
		<< endl
		<< "Options:" << endl
		<< "\t" << OPTION_PROBLEM << "\t" << "Sub problems." << endl
		<< "\t\t" << "You can choose from \"" << PROBLEM_A << "\"(A - geometric transformation), \"" << PROBLEM_B << "\"(B - spatial warpping), \"" << PROBLEM_C << "\"(C - lens distortion correction)." << endl
		<< "\t\t" <<"NOTE: If you choose problem A, the input should be the folder that contains images, otherwise the input image filename." << endl
		<< "\t\t" << "The default is Problem A." << endl
		<< "\t" << OPTION_SHRINK << "\t" << "# of shrinking pixels of the auto detected boundaries of patterns. For problwm A. The default is " << DEFAULT_SHRINK << "." << endl
		<< "\t" << OPTION_OUTPUT << "\t" << "Output filename. The default is \"" << DEFAULT_OUTPUT_FILENAME << "\"." << endl
		<< "\t" << OPTION_HEIGHT << "\t" << "Height of the input image. The default is " << DEFAULT_HEIGHT << "." << endl
		<< "\t" << OPTION_WIDTH << "\t" << "Width of the input image. The default is " << DEFAULT_WIDTH << "." << endl
		<< "\t" << OPTION_CHANNEL << "\t" << "Number of channels of the input image. The default is " << GRAY_CHANNELS << "." << endl
		<< endl
		<< "Example:" << endl
		<< "\t" << "Problem1 -f " << PROBLEM_B << " -o my_output_image.raw my_input_image.raw" << endl
		<< endl;
}

int main(int argc, char *argv[]) {
	auto problem = DEFAULT_PROBLEM;
	auto shrink = DEFAULT_SHRINK;
	auto output = string(DEFAULT_OUTPUT_FILENAME);
	auto height = DEFAULT_HEIGHT;
	auto width = DEFAULT_WIDTH;
	auto channel = GRAY_CHANNELS;

	auto problemFlag = false;
	auto shrinkFlag = false;
	auto outputFlag = false;
	auto heightFlag = false;
	auto widthFlag = false;
	auto channelFlag = false;
	auto input = string();

#if defined(DEBUG) || defined(_DEBUG)
	cerr << "WARNNING: You are running this program under DEBUG mode which is extremely SLOW! RELEASE mode will give you several handurd speed up in this problem." << endl << endl;
#endif 

	try {
		int i;
		for (i = 1; i < argc; i++) {
			auto cmd = string(argv[i]);
			if (problemFlag) {
				problem = ParseFunction(cmd);
				problemFlag = false;
			} else if (shrinkFlag) {
				shrink = atoi(cmd.c_str());
				shrinkFlag = false;
			} else if (outputFlag) {
				output = cmd;
				outputFlag = false;
			} else if (heightFlag) {
				height = atoi(cmd.c_str());
				heightFlag = false;
			} else if (widthFlag) {
				width = atoi(cmd.c_str());
				widthFlag = false;
			} else if (channelFlag) {
				channel = atoi(cmd.c_str());
				channelFlag = false;
			} else if (cmd == OPTION_PROBLEM) {
				problemFlag = true;
			} else if (cmd == OPTION_SHRINK) {
				shrinkFlag = true;
			} else if (cmd == OPTION_OUTPUT) {
				outputFlag = true;
			} else if (cmd == OPTION_HEIGHT) {
				heightFlag = true;
			} else if (cmd == OPTION_WIDTH) {
				widthFlag = true;
			} else if (cmd == OPTION_CHANNEL) {
				channelFlag = true;
			} else {
				input = cmd;
				break;
			}
		}
		if (input == "" || i != argc - 1 || problemFlag || shrinkFlag || outputFlag || heightFlag || widthFlag || channelFlag) {
			PrintUsage();
			throw WrongCommandException;
		}

		Image<unsigned char> in;
		Image<unsigned char> out;

		switch (problem) {
			case Problem::A:				
				out = FillHoles(input, shrink);
				break;
			case Problem::B:
				in = Image<unsigned char>(height, width, channel, input);;
				out = Warp(in);
				break;
			case Problem::C:
				cerr << "WARRNING: This sub problem c is implemented in MATLAB, run \"problem1c.m\" instead!" << endl
					<< " This C++ code is just for an experimental method. I did not use C++ to implement this sub problem, because no external C++ libraires will benifit both me and the grader." << endl
					<< " And I do not want to write mathematical functions by hand, since there is no bounus for that. TA said in this sub problem we can use other mathematical tools." << endl << endl;
				in = Image<unsigned char>(height, width, channel, input);;
				out = LensDistrotionCorrection(in);
				break;
			default:
				throw InvalidOperationException;
		}

		if (out.getHeight() * out.getWidth() > 0) {
			out.writeToFile(output);
		}
		return 0;
	} catch (const char * ex) {
		cerr << "Captured exception: " << ex << endl;
	}
	return 1;
}