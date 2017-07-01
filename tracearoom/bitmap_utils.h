#pragma once

#include<array>
#include<fstream>
#include<memory>
#include<string>

namespace bitmap_utils
{
	class bitmap_image
	{
		typedef unsigned char uchar;

		// Padding could affect the size of the struct depending upon the machine
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

		std::array<uchar, 14> file_header = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
		std::unique_ptr<uchar[]> pix_data = nullptr;

		uint32_t width;
		uint32_t height;

		// Flips the pixel data horizontally(by rows)
		void flip_pixel_data()
		{
			uint32_t current_row = 0;
			uint32_t pivot = (height % 2 == 0) ? (height / 2 - 1) : (height / 2);

			for (uint32_t idx = 0; idx <= pivot; ++idx)
			{
				std::unique_ptr<uchar[]> temp_arrray(new uchar[height]);

				uint32_t upper_row_start_idx = idx * width * 3;
				uint32_t upper_row_end_idx = upper_row_start_idx + width * 3;
				uint32_t lower_row_start_idx = (height - 1 - idx) * width * 3;

				std::swap_ranges(&pix_data[upper_row_start_idx], &pix_data[upper_row_end_idx], &pix_data[lower_row_start_idx]);
			}
		}
	public:
		bitmap_image(uint32_t img_width, uint32_t img_height, std::unique_ptr<uchar[] > pixel_data) : width(img_width), height(img_height), pix_data(std::move(pixel_data))
		{
			info_header.width = width;
			info_header.height = height;

			uint32_t file_size = width * height * 3 + 54;
			uchar* pFH = reinterpret_cast<uchar*>(&file_size);

			// Assign the bytes to size field of bitmpa file header
			file_header[2] = pFH[0], file_header[3] = pFH[1], file_header[4] = pFH[2], file_header[5] = pFH[3];
		}

		void write_to_file(std::string file_path)
		{
			uchar infoheader_array[40];
			std::memcpy(infoheader_array, &info_header, 40);
			flip_pixel_data();

			std::ofstream ofs(file_path);
			ofs.write((char *)file_header.data(), 14);
			ofs.write((char *)infoheader_array, 40);
			ofs.write((char*)pix_data.get(), width * height * 3);

			ofs.close();
		}
	};
}