#pragma once

#include<fstream>
#include<cinttypes>
#include<cstring>
#include<string>
#include<vector>
#include<iterator>
#include<streambuf>
#include<memory>

class ppm_to_bmp
{
	typedef unsigned char uchar;
	std::string ppm_path;
	uint32_t file_size, ppm_width, ppm_height, max_intensity;
	uchar *image_data;
	struct BITMAPINFOHEADER
	{
		uint32_t size = 40;
		uint32_t width;
		uint32_t height;
		uint16_t planes = 1;
		uint16_t bitCount = 24; //24 bits per pixel
		uint32_t compression = 0;
		uint32_t imageDataSize = 0; // If compression is zero this can be set to 0
		uint32_t xPixelsPerMetre = 0;
		uint32_t yPixelsPerMetre = 0;
		uint32_t nColorsUsed = 0;
		uint32_t colorsImp = 0; // all colors important
	} info_header;

	void update_bmp_header()
	{
		info_header.width = ppm_width;
		info_header.height = ppm_height;
	}
	void read_data()
	{
		std::ifstream ifs(ppm_path, std::ios::in);
		std::basic_string<uchar> file_contents;
		ifs.seekg(0, std::ios::end);
		file_contents.reserve(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		file_contents.assign((std::istream_iterator<uchar>(ifs)),
			std::istream_iterator<uchar>());

		if (!ifs.is_open())
		{
			//Send error to application
			//   std::cout << "Unable to open ppm file. Please check the path.\n";
			exit(1);
		}
		//char signature[2];
		//ifs >> signature[0], ifs >> signature[1];
		//if (signature[0] != 'P' || signature[1] != '6')
		//{
		//	// std::cerr<<"Incorrect header\n";
		//	exit(1);
		//}
		//ifs >> ppm_width, ifs >> ppm_height, ifs >> max_intensity;
		//ifs.close();
		//ifs.open(ppm_path, std::ios::in | std::ios::binary);
		//auto stream_start = std::istreambuf_iterator<char>(ifs);
		//auto stream_end = std::istreambuf_iterator<char>();
		//std::vector<uchar> data(stream_start, stream_end);
		//ifs.close();
		//file_size = ppm_width * ppm_height * 3 + 54;
		//image_data = new uchar[file_size - 54];
		//uchar *des_start = image_data;
		//for (auto src_start = data.end() - ppm_width * 3; src_start > data.begin() + 15; src_start -= ppm_width * 3)
		//{
		//	auto src_end = src_start + ppm_width * 3;
		//	std::copy(src_start, src_end, des_start);
		//	des_start += ppm_width * 3;
		//}
	}
public:
	 ppm_to_bmp(std::string p_path) : ppm_path(p_path), file_size(0) { read_data(); }
	~ppm_to_bmp() 
	{ 
		//delete[] image_data; 
	}

	void convert_to_bmp(std::string bmp_path)
	{
		update_bmp_header();
		std::ofstream ofs(bmp_path, std::ios::binary | std::ios::out);
		uchar ih_array[40];
		std::memcpy(ih_array, &info_header, 40);
		uchar fH[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
		uchar* pFH = reinterpret_cast<uchar*>(&file_size);
		fH[2] = pFH[0], fH[3] = pFH[1], fH[4] = pFH[2], fH[5] = pFH[3];

		ofs.write((char*)fH, 14);
		ofs.write((char*)ih_array, 40);
		ofs.write((char*)image_data, ppm_width * ppm_height * 3);
		ofs.close();
	}

	void write_bmp_data(std::ostream &ofs, int width, int height, std::unique_ptr<char []> &img_data)
	{
		uint32_t fsize = width * height * 3 + 54;
		info_header.width = width;
		info_header.height = height;
		uchar ih_array[40];
		std::memcpy(ih_array, &info_header, 40);
		uchar fH[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
		uchar* pFH = reinterpret_cast<uchar*>(&fsize);
		fH[2] = pFH[0], fH[3] = pFH[1], fH[4] = pFH[2], fH[5] = pFH[3];

		ofs.write((char*)fH, 14);
		ofs.write((char*)ih_array, 40);
		ofs.write((char*)img_data.get(), width * height * 3);
	}
};
