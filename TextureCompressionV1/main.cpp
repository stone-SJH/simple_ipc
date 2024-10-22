#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <fstream>

#include "astcenc.h"
#include "astcenccli_internal.h"
#include "ispc_texcomp.h"

#include "../ipc/ipc.h"
#include "../tls/IPCUtility.h"
#include <iostream>
#include <string>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION 
#include "stb_image_resize.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"

int Compress(int argc, char** argv, IPC_Stream* ipcStream);
int CompressASTC(int argc, char** argv, IPC_Stream* ipcStream);
int CompressBC(int argc, char** argv, IPC_Stream* ipcStream);

void write_dds(const char* filename, int width, int height, const void* data, size_t dataSize, int bcN, IPC_Stream* ipcStream, int mipCount = 0, bool isFirstMipLevel = true);

int write_astc(const char* filename, const astc_compressed_image& img, IPC_Stream* ipcStream);
int write_astc_header(const char* filename, const astc_compressed_image& img, IPC_Stream* ipcStream);
int write_astc_data(const char* filename, const astc_compressed_image& img, bool isAppend, IPC_Stream* ipcStream);

void buffer_astc_header(std::vector<uint8_t>& buffer, const astc_compressed_image& img);
void buffer_astc_data(std::vector<uint8_t>& buffer, const astc_compressed_image& img);

#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define DDS_MAGIC 0x20534444  // "DDS "

#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_LINEARSIZE 0x80000

#define DDPF_FOURCC 0x4

#define DDSCAPS_TEXTURE 0x1000

#define DXGI_FORMAT_BC7_UNORM 98
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D 3

#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP 0x400000
#define DDSD_MIPMAPCOUNT 0x20000


typedef struct {
	uint32_t       dwSize;
	uint32_t       dwFlags;
	uint32_t       dwHeight;
	uint32_t       dwWidth;
	uint32_t       dwPitchOrLinearSize;
	uint32_t       dwDepth;
	uint32_t       dwMipMapCount;
	uint32_t       dwReserved1[11];
	struct {
		uint32_t   dwSize;
		uint32_t   dwFlags;
		uint32_t   dwFourCC;
		uint32_t   dwRGBBitCount;
		uint32_t   dwRBitMask;
		uint32_t   dwGBitMask;
		uint32_t   dwBBitMask;
		uint32_t   dwABitMask;
	} ddpfPixelFormat;
	struct {
		uint32_t   dwCaps1;
		uint32_t   dwCaps2;
		uint32_t   dwDDSX;
		uint32_t   dwReserved;
	} ddsCaps;
	uint32_t       dwReserved2;
} DDS_HEADER;

typedef struct {
	uint32_t dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
} DDS_HEADER_DXT10;


void write_dds(const char* filename, int width, int height, const void* data, size_t dataSize, int bcN, IPC_Stream* ipcStream, int mipCount, bool isFirstMipLevel)
{
	FILE* file;
	if (mipCount == 1 || isFirstMipLevel)
	{
		file = fopen(filename, "wb");
		if (!file) 
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to open output file: " +
				std::string(filename) + ". Ensure the file path is correct and the directory is writable.\n");
			return;
		}

		uint32_t ddsMagic = DDS_MAGIC;
		fwrite(&ddsMagic, sizeof(uint32_t), 1, file);

		DDS_HEADER header = { 0 };
		header.dwSize = 124;
		header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
		header.dwHeight = height;
		header.dwWidth = width;
		header.dwPitchOrLinearSize = dataSize;
		header.dwDepth = 0;
		header.dwMipMapCount = 0;
		header.ddpfPixelFormat.dwSize = 32;
		header.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		if (bcN == 7) 
		{
			header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', '1', '0');
		}
		else if (bcN == 3) 
		{
			header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '5');
		}
		else if (bcN == 1) 
		{
			header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1');
		}
		header.ddsCaps.dwCaps1 = DDSCAPS_TEXTURE;
		if (mipCount > 1)
		{
			header.dwFlags |= DDSD_MIPMAPCOUNT;
			header.dwMipMapCount = mipCount;
			header.ddsCaps.dwCaps1 |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
		}		

		fwrite(&header, sizeof(DDS_HEADER), 1, file);

		if (bcN == 7) {
			DDS_HEADER_DXT10 dxt10Header = {
				DXGI_FORMAT_BC7_UNORM,
				D3D10_RESOURCE_DIMENSION_TEXTURE2D,
				0,
				1,
				0
			};
			fwrite(&dxt10Header, sizeof(DDS_HEADER_DXT10), 1, file);
		}
	}
	else 
	{
		file = fopen(filename, "ab");
		if (!file) 
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to open output file for appending: " +
				std::string(filename) + ". Ensure the file path is correct and the directory is writable.\n");
			return;
		}
	}

	fwrite(data, 1, dataSize, file);
	fclose(file);
}


uint8_t* generate_mipmap(uint8_t* input, int input_width, int input_height, int channels, int* output_width, int* output_height) 
{
	*output_width = input_width / 2;
	*output_height = input_height / 2;
	if (*output_width == 0) *output_width = 1;
	if (*output_height == 0) *output_height = 1;

	uint8_t* output = (uint8_t*)malloc(*output_width * *output_height * channels);
	stbir_resize_uint8(input, input_width, input_height, 0, output, *output_width, *output_height, 0, channels);
	return output;
}

int main(int argc, char** argv)
{
	//std::string msg = "tc astc 4x4 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output.astc";

	if (argc < 2)
	{
		fprintf(stderr, "Usage: <pipename>\n");
		return 1;
	}
	char* ipcStreamName = argv[1];

	//char* ipcStreamName = "yatc-001";

	IPC_Stream* ipcStream;
	IPC_ErrorCode error = IPC_Succeeded;
	ipcStream = IPC_Create(&error);
	if (error != IPC_Succeeded)
		return 1;
	bool connected = false;
	int retry = 0;

	while (!connected && retry < 100) {
		IPC_ConnectTo(ipcStream, ipcStreamName, &error);
		if (error == IPC_Succeeded ||
			error == IPC_AlreadyConnected)
		{
			connected = true;
		}
		else {
			retry++;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "retry " << retry << " time..." << std::endl;
		}
	}

	if (!connected) {
		IPC_Destroy(ipcStream, &error);
		return 2;
	}

	std::string msg;
	int ret = -1;

	try {
		while (1) {
			{
				bool ok = SafeRead(ipcStream, msg);
				if (!ok) {
					// ipc reading error 
					break;
				}
				if (msg.empty()) {
					// ipc connection closed
					ret = 3;
					break;
				}
				if (msg == "shutdown") {
					// clean shutdown
					std::cout << "shutdown" << std::endl;
					ret = 0;
					break;
				}
				if (msg == "202405230000heartbeat") {
					WriteString(ipcStream, "202405230000heartbeat");
					continue;
				}
				//do something special here
				std::stringstream ss(msg);
				std::vector<std::string> tokens;
				std::string token;

				while (ss >> token) {
					tokens.push_back(token);
				}

				// Convert the vector of strings to char** argv
				int compressArgc = tokens.size();
				char** compressArgv = new char* [compressArgc];

				for (int i = 0; i < compressArgc; i++) {
					compressArgv[i] = new char[tokens[i].length() + 1];  // +1 for null-terminator
					std::strcpy(compressArgv[i], tokens[i].c_str());
				}
				
				if (Compress(compressArgc, compressArgv, ipcStream) == 0)
					WriteString(ipcStream, "Job done.");
				else
					WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to process the command due to internal errors.\n");
			}
		}
	}
	catch (IPCException& e) {
		std::cout << "Unhandled tc exception: " << e.what() << " (error " << (uint32_t)(e.GetData().m_ErrorID) << ", transferred " << e.GetData().m_BytesCompleted << '/' << e.GetData().m_BytesTotal << ')' << std::endl;
	}

	error = IPC_Succeeded;
	IPC_Disconnect(ipcStream, &error);
	IPC_Destroy(ipcStream, &error);
}

int Compress(int argc, char** argv, IPC_Stream* ipcStream) {
	if (argc < 5)
	{
		WriteString(ipcStream, "[YahahaTextureCompression]: Usage - Correct format: <format> <options> <input file> <output file>\n");
		return 1;
	}

	const char* format = argv[1];
	if (strcmp(format, "astc") == 0)
	{
		// Example: astc 6x6 input.png output.astc
		const char* blockSize = argv[2];
		const char* inputFile = argv[3];
		const char* outputFile = argv[4];

		if (strcmp(blockSize, "4x4") != 0 && strcmp(blockSize, "5x5") != 0 && strcmp(blockSize, "6x6") != 0)
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Unsupported ASTC block size specified. Valid block sizes are 4x4, 5x5, or 6x6.\n");
			return 1;
		}
		if (strstr(outputFile, ".astc") == NULL)
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Incorrect file extension for output file. Expected a .astc extension for ASTC format.\n");
			return 1;
		}
		return CompressASTC(argc - 2, argv + 2, ipcStream);
	}
	else if (strcmp(format, "bc") == 0)
	{
		// Example: bc 7 input.png output.dds
		const char* version = argv[2];
		const char* inputFile = argv[3];
		const char* outputFile = argv[4];

		if (strcmp(version, "1") != 0 && strcmp(version, "3") != 0 && strcmp(version, "7") != 0)
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Unsupported BC compression version specified. Please specify one of the following supported versions: bc 1, bc 3, or bc 7.\n");
			return 1;
		}
		if (strstr(outputFile, ".dds") == NULL)
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Incorrect file extension for output file. Expected a .dds extension for BC format.\n");
			return 1;
		}
		return CompressBC(argc - 2, argv + 2, ipcStream);
	}
	else
	{
		WriteString(ipcStream, "[YahahaTextureCompression]: Error - Unsupported format specified. Please use 'astc' or 'bc'.\n");
		return 1;
	}
	return 0;
}

#define MAX_MIPMAP_LEVELS 20
int max(int a, int b) {
	return (a > b) ? a : b;
}

int CompressBC(int argc, char** argv, IPC_Stream* ipcStream)
{
	const char* version = argv[0];
	int bcN = 0;
	if (strcmp(version, "1") == 0)
		bcN = 1;
	else if (strcmp(version, "3") == 0)
		bcN = 3;
	else if (strcmp(version, "7") == 0)
		bcN = 7;

	bool generateMipmaps = false;
	for (int i = 3; i < argc; i++)
	{
		if (strcmp(argv[i], "-mipmap") == 0)
			generateMipmaps = true;
	}

	int image_x, image_y, image_c;
	uint8_t* image_data = (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);

	rgba_surface surfaceInput;
	surfaceInput.ptr = image_data;
	surfaceInput.stride = image_x * 4;
	surfaceInput.width = image_x;
	surfaceInput.height = image_y;

	rgba_surface mipmaps[MAX_MIPMAP_LEVELS];
	mipmaps[0] = surfaceInput;
	int num_levels = 1;

	if (generateMipmaps)
	{
		//int channels = format == 1 ? 3 : 4;
		int channels = 4;
		while (mipmaps[num_levels - 1].width > 1 || mipmaps[num_levels - 1].height > 1)
		{
			int new_width = max(1, mipmaps[num_levels - 1].width / 2);
			int new_height = max(1, mipmaps[num_levels - 1].height / 2);
			mipmaps[num_levels].ptr = generate_mipmap(mipmaps[num_levels - 1].ptr, mipmaps[num_levels - 1].width, mipmaps[num_levels - 1].height, channels, &new_width, &new_height);
			mipmaps[num_levels].width = new_width;
			mipmaps[num_levels].height = new_height;
			mipmaps[num_levels].stride = new_width * 4;
			num_levels++;
			if (new_width == 1 && new_height == 1)
				break;
		}
	}

	for (int i = 0; i < num_levels; i++)
	{
		size_t num_blocks_wide = (mipmaps[i].width + 3) / 4;
		size_t num_blocks_high = (mipmaps[i].height + 3) / 4;
		size_t output_buffer_size;
		if (bcN == 1)
			output_buffer_size = num_blocks_wide * num_blocks_high * 8;
		else
			output_buffer_size = num_blocks_wide * num_blocks_high * 16;
		uint8_t* output_buffer = (uint8_t*)malloc(output_buffer_size);

		switch (bcN)
		{
		case 1:
			CompressBlocksBC1(&mipmaps[i], output_buffer);
			break;
		case 3:
			CompressBlocksBC3(&mipmaps[i], output_buffer);
			break;
		case 7:
			bc7_enc_settings encodingSettings;
			GetProfile_basic(&encodingSettings);
			CompressBlocksBC7(&mipmaps[i], output_buffer, &encodingSettings);
			break;
		}

		bool isFirstMipLevel = i == 0;
		write_dds(argv[2], mipmaps[i].width, mipmaps[i].height, output_buffer, output_buffer_size, bcN, ipcStream, num_levels, isFirstMipLevel);

		free(output_buffer);
		if (i != 0) 
			free(mipmaps[i].ptr);
	}
	stbi_image_free(image_data);
	return 0;
}

int CompressASTC(int argc, char** argv, IPC_Stream* ipcStream)
{
	static unsigned int thread_count = get_cpu_count();
	//static const unsigned int block_x = 4;
	//static const unsigned int block_y = 4;
	static const unsigned int block_z = 1;
	static const astcenc_profile profile = ASTCENC_PRF_LDR;
	static const float quality = ASTCENC_PRE_MEDIUM;
	static const astcenc_swizzle swizzle{
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	bool generateMipmaps = false;
	bool memoryMode = false;
	for (int i = 3; i < argc; i++)
	{
		if (strcmp(argv[i], "-mipmap") == 0)
			generateMipmaps = true;
		else if (strcmp(argv[i], "-background") == 0)
			thread_count = std::min(5u, thread_count);
		else if (strcmp(argv[i], "-threads") == 0 && i + 1 < argc)
		{  
			int temp = atoi(argv[i + 1]);
			if (temp > 0) 
			{  
				thread_count = static_cast<unsigned int>(temp);
				++i;
			}
		}
		else if (strcmp(argv[i], "-memoryMode") == 0)
			memoryMode = true;
	}

	std::vector<uint8_t> ipc_buffer;
	unsigned int block_x, block_y;
	sscanf(argv[0], "%ux%u", &block_x, &block_y); 

	astcenc_config config;
	config.block_x = block_x;
	config.block_y = block_y;
	config.block_z = block_z;
	config.profile = profile;

	astcenc_error status;
	status = astcenc_config_init(profile, block_x, block_y, block_z, quality, 0, &config);

	astcenc_context* context;
	status = astcenc_context_alloc(&config, thread_count, &context);

	bool isHdr;
	unsigned int componentCount;
	astcenc_image* image_uncomp_in;
	int dim_x, dim_y;
	bool y_flip = false;
	if (stbi_is_hdr(argv[1]))
	{
		float* data = stbi_loadf(argv[1], &dim_x, &dim_y, nullptr, STBI_rgb_alpha);
		if (data)
		{
			image_uncomp_in = astc_img_from_floatx4_array(data, dim_x, dim_y, y_flip);
			stbi_image_free(data);
		}
	}
	else
	{
		uint8_t* data = stbi_load(argv[1], &dim_x, &dim_y, nullptr, STBI_rgb_alpha);
		if (data)
		{
			image_uncomp_in = astc_img_from_unorm8x4_array(data, dim_x, dim_y, y_flip);
			stbi_image_free(data);
		}
	}

	double image_size = static_cast<double>(image_uncomp_in->dim_x) *
		static_cast<double>(image_uncomp_in->dim_y) *
		static_cast<double>(image_uncomp_in->dim_z);

	astcenc_image* mipmaps[MAX_MIPMAP_LEVELS];
	mipmaps[0] = image_uncomp_in;
	int num_levels = 1;
	if (generateMipmaps)
	{
		int channels = 4;
		while (mipmaps[num_levels - 1]->dim_x > 1 || mipmaps[num_levels - 1]->dim_y > 1)
		{
			int new_width = max(1, mipmaps[num_levels - 1]->dim_x / 2);
			int new_height = max(1, mipmaps[num_levels - 1]->dim_y / 2);

			uint8_t* output = (uint8_t*)malloc(new_width * new_width * channels);
			stbir_resize_uint8(static_cast<uint8_t*>(mipmaps[num_levels - 1]->data[0]), 
				mipmaps[num_levels - 1]->dim_x, mipmaps[num_levels - 1]->dim_y, 0, output, new_width, new_height, 0, channels);
			mipmaps[num_levels] = astc_img_from_unorm8x4_array(output, new_width, new_height, y_flip);
			
			num_levels++;
			if (new_width == 1 && new_height == 1)
				break;
		}
	}
	for (int i = 0; i < num_levels; i++)
	{
		unsigned int blocks_x = (mipmaps[i]->dim_x + block_x - 1) / block_x;
		unsigned int blocks_y = (mipmaps[i]->dim_y + block_y - 1) / block_y;
		size_t buffer_size = blocks_x * blocks_y * 16;
		uint8_t* buffer = new uint8_t[buffer_size];


		compression_workload work;
		work.context = context;
		work.image = mipmaps[i];
		work.swizzle = swizzle;
		work.data_out = buffer;
		work.data_len = buffer_size;
		work.error = ASTCENC_SUCCESS;

		if (thread_count > 1)
		{
			launch_threads("Compression", thread_count, compression_workload_runner, &work);
		}
		else
		{
			work.error = astcenc_compress_image(context, work.image, &swizzle, work.data_out, work.data_len, 0);
		}

		//status = astcenc_compress_image(work.context, mipmaps[i], &work.swizzle, buffer, buffer_size, 0);
		if (work.error != ASTCENC_SUCCESS)
		{
			WriteString(ipcStream, "[YahahaTextureCompression]: Error - Codec compression failed: " + std::string(astcenc_get_error_string(status)));
			return 1;
		}

		astc_compressed_image image_comp{};
		image_comp.block_x = config.block_x;
		image_comp.block_y = config.block_y;
		image_comp.block_z = config.block_z;
		image_comp.dim_x = mipmaps[i]->dim_x;
		image_comp.dim_y = mipmaps[i]->dim_y;
		image_comp.dim_z = mipmaps[i]->dim_z;
		image_comp.data = buffer;
		image_comp.data_len = buffer_size;

		if (memoryMode == false)
		{
			int error;
			if (i == 0)
			{
				error = write_astc_header(argv[2], image_comp, ipcStream);
			}
			error = write_astc_data(argv[2], image_comp, true, ipcStream);
			if (error)
			{
				WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to store the compressed image.\n");
				return 1;
			}
		}
		else
		{
			if (i == 0) {
				buffer_astc_header(ipc_buffer, image_comp);
			}
			buffer_astc_data(ipc_buffer, image_comp);
		}

		astcenc_compress_reset(context);

		free(buffer);
		free_image(mipmaps[i]);
		//stbi_image_free(image_data);
	}

	if (memoryMode)
	{
		WriteBuffer(ipcStream, static_cast<const void*>(ipc_buffer.data()), ipc_buffer.size());
	}

	astcenc_context_free(context);
	return 0;
}

struct Astc_header
{
	uint8_t magic[4];
	uint8_t block_x;
	uint8_t block_y;
	uint8_t block_z;
	uint8_t dim_x[3];			// dims = dim[0] + (dim[1] << 8) + (dim[2] << 16)
	uint8_t dim_y[3];			// Sizes are given in texels;
	uint8_t dim_z[3];			// block count is inferred
};
static const uint32_t ASTC_MAGIC_ID = 0x5CA1AB13;

int write_astc(const char* filename, const astc_compressed_image& img, IPC_Stream* ipcStream)
{

	Astc_header hdr;
	hdr.magic[0] = ASTC_MAGIC_ID & 0xFF;
	hdr.magic[1] = (ASTC_MAGIC_ID >> 8) & 0xFF;
	hdr.magic[2] = (ASTC_MAGIC_ID >> 16) & 0xFF;
	hdr.magic[3] = (ASTC_MAGIC_ID >> 24) & 0xFF;

	hdr.block_x = static_cast<uint8_t>(img.block_x);
	hdr.block_y = static_cast<uint8_t>(img.block_y);
	hdr.block_z = static_cast<uint8_t>(img.block_z);

	hdr.dim_x[0] = img.dim_x & 0xFF;
	hdr.dim_x[1] = (img.dim_x >> 8) & 0xFF;
	hdr.dim_x[2] = (img.dim_x >> 16) & 0xFF;

	hdr.dim_y[0] = img.dim_y & 0xFF;
	hdr.dim_y[1] = (img.dim_y >> 8) & 0xFF;
	hdr.dim_y[2] = (img.dim_y >> 16) & 0xFF;

	hdr.dim_z[0] = img.dim_z & 0xFF;
	hdr.dim_z[1] = (img.dim_z >> 8) & 0xFF;
	hdr.dim_z[2] = (img.dim_z >> 16) & 0xFF;

	FILE* file = fopen(filename, "wb");
	if (file == NULL)
	{
		WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to open output file: " + 
			std::string(filename) + ". Ensure the file path is correct and the directory is writable.\n");
		return 1;
	}

	// 写入头部信息
	fwrite(&hdr, sizeof(Astc_header), 1, file);
	// 写入图片数据
	fwrite(img.data, img.data_len, 1, file);

	fclose(file);
	return 0;
}

int write_astc_header(const char* filename, const astc_compressed_image& img, IPC_Stream* ipcStream)
{
	Astc_header hdr;
	hdr.magic[0] = ASTC_MAGIC_ID & 0xFF;
	hdr.magic[1] = (ASTC_MAGIC_ID >> 8) & 0xFF;
	hdr.magic[2] = (ASTC_MAGIC_ID >> 16) & 0xFF;
	hdr.magic[3] = (ASTC_MAGIC_ID >> 24) & 0xFF;

	hdr.block_x = static_cast<uint8_t>(img.block_x);
	hdr.block_y = static_cast<uint8_t>(img.block_y);
	hdr.block_z = static_cast<uint8_t>(img.block_z);

	hdr.dim_x[0] = img.dim_x & 0xFF;
	hdr.dim_x[1] = (img.dim_x >> 8) & 0xFF;
	hdr.dim_x[2] = (img.dim_x >> 16) & 0xFF;

	hdr.dim_y[0] = img.dim_y & 0xFF;
	hdr.dim_y[1] = (img.dim_y >> 8) & 0xFF;
	hdr.dim_y[2] = (img.dim_y >> 16) & 0xFF;

	hdr.dim_z[0] = img.dim_z & 0xFF;
	hdr.dim_z[1] = (img.dim_z >> 8) & 0xFF;
	hdr.dim_z[2] = (img.dim_z >> 16) & 0xFF;

	FILE* file = fopen(filename, "wb");
	if (file == NULL)
	{
		WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to open output file: " +
			std::string(filename) + ". Ensure the file path is correct and the directory is writable.\n");
		return 1;
	}	
	fwrite(&hdr, sizeof(Astc_header), 1, file);
	fclose(file);
	return 0;
}

//后面如果只写data，不写头，就可以用到
int write_astc_data(const char* filename, const astc_compressed_image& img, bool isAppend, IPC_Stream* ipcStream)
{
	FILE* file;// = fopen(filename, "wb");	
	if (isAppend)
		file = fopen(filename, "ab");
	else
		file = fopen(filename, "wb");
	if (file == NULL)
	{
		WriteString(ipcStream, "[YahahaTextureCompression]: Error - Failed to open output file: " +
			std::string(filename) + ". Ensure the file path is correct and the directory is writable.\n");
		return 1;
	}
	fwrite(img.data, img.data_len, 1, file);
	fclose(file);
	return 0;
}

void buffer_astc_header(std::vector<uint8_t>& buffer, const astc_compressed_image& img) 
{
	Astc_header hdr;
	hdr.magic[0] = ASTC_MAGIC_ID & 0xFF;
	hdr.magic[1] = (ASTC_MAGIC_ID >> 8) & 0xFF;
	hdr.magic[2] = (ASTC_MAGIC_ID >> 16) & 0xFF;
	hdr.magic[3] = (ASTC_MAGIC_ID >> 24) & 0xFF;

	hdr.block_x = static_cast<uint8_t>(img.block_x);
	hdr.block_y = static_cast<uint8_t>(img.block_y);
	hdr.block_z = static_cast<uint8_t>(img.block_z);

	hdr.dim_x[0] = img.dim_x & 0xFF;
	hdr.dim_x[1] = (img.dim_x >> 8) & 0xFF;
	hdr.dim_x[2] = (img.dim_x >> 16) & 0xFF;

	hdr.dim_y[0] = img.dim_y & 0xFF;
	hdr.dim_y[1] = (img.dim_y >> 8) & 0xFF;
	hdr.dim_y[2] = (img.dim_y >> 16) & 0xFF;

	hdr.dim_z[0] = img.dim_z & 0xFF;
	hdr.dim_z[1] = (img.dim_z >> 8) & 0xFF;
	hdr.dim_z[2] = (img.dim_z >> 16) & 0xFF;

	uint8_t* ptr = reinterpret_cast<uint8_t*>(&hdr);
	buffer.insert(buffer.end(), ptr, ptr + sizeof(hdr));
}

void buffer_astc_data(std::vector<uint8_t>& buffer, const astc_compressed_image& img) 
{
	buffer.insert(buffer.end(), img.data, img.data + img.data_len);
}