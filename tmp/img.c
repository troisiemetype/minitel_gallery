#include "minitel_bt_images.h"

File newFile;

JPEGDEC img;

imgState_t *imgState = NULL;

void img_logRawBytes(const uint8_t *data, uint16_t length){
	for(int16_t i = 0; i < length; ++i){
		printf("0x%02x ", data[i]);
		if(((i + 1) % 16) == 0) printf("\n");
	}
	printf("\n");

}

void* img_openCB(const char *filename, int32_t *size){
	log_d("CB : open image");
	newFile = SD.open(filename);
	if(newFile){
		*size = newFile.size();
		*imgState = PROCESSING_IMG;
		log_d("image opened");
	} else {
		// What do we do when the picture cannot be opened ?
		log_e("couln't open image");
	}

	return &newFile;
}

void img_closeCB(void *handle){
	log_d("CB : close image");
	if(newFile) newFile.close();
}

int32_t img_readCB(JPEGFILE *file, uint8_t *buf, int32_t len){
	log_d("CB : read %i bytes of image", len);

	if(!newFile){
		log_e(" file error");
		return 0;
	}
	return newFile.read(buf, len);
}

int32_t img_seekCB(JPEGFILE *file, int32_t pos){
	log_d("CB : seek image at %i", pos);
	if(!newFile){
		log_e(" file error");
		return 0;
	}
	return newFile.seek(pos);
}

int img_decodeCB(JPEGDRAW *draw){
	log_d("decoding block");
	log_d("position : %i; %i", draw->x, draw->y);
	log_d("size : %i x %i", draw->iWidth, draw->iHeight);
	uint16_t size = draw->iWidth * draw->iHeight;
	log_d("%i 16-bit words", size);

//	img_logRawBytes((uint8_t*)draw->pPixels, size);

	return 1;
}

void img_decode(const char* file, imgState_t* state){

	imgState = state;
	// open(fileName, openCB, closeCB, readCB, seekCB, drawCB)
	int ret = img.open(file, img_openCB, img_closeCB, img_readCB, img_seekCB, img_decodeCB);
	if(ret){
		uint16_t width = img.getWidth();
		uint16_t height = img.getHeight();
		log_d("new image %i x %i", width, height);

		/*
		 *	Pixel type can be :
		 *		RGB565_LITTLE_ENDIAN (default)
		 *		RGB565_BIG_ENDIAN
		 *		EIGHT_BIT_GRAYSCALE
		 *		FOUR_BIT_DITHERED
		 *		TWO_BIT_DITHERED
		 *		ONE_BIT_DITHERED
		 *
		 *	Options for decode are (those are bit masks):
		 *		JPEG_AUTO_ROTATE
		 *		JPEG_SCALE_HALF
		 *		JPEG_SCALE_QUARTER
		 *		JPEG_SCALE_EIGHTH
		 *		JPEG_LE_PIXELS			// can't find it used anywhere
		 *		JPEG_EXIF_THUMBNAIL
		 *		JPEG_LUMA_ONLY			// can't find it used anywhere
		 */

		float widthDivider = 1;
		float heightDivider = 1;

		float divider = 1;

		widthDivider = (float)width / MINITEL_MAX_WIDTH;
		heightDivider = (float)height / MINITEL_MAX_HEIGHT;

		log_d("width divider : %.2f", widthDivider);
		log_d("height divider : %.2f", heightDivider);

		if(widthDivider > heightDivider){
			log_d("keeping height");
			divider = heightDivider;
		} else {
			log_d("keeping width");
			divider = widthDivider;
		}
		if(divider < 1) divider = 1;

		uint16_t newWidth = width / divider;
		uint16_t newHeight = height / divider;
		log_d("image new size : %ix%i, with divider %.2f divider", newWidth, newHeight, divider);

		useDither = false;
		if(useDither){
			// Dither decoding
			log_d("decoding and dithering image");
			uint8_t *data = (uint8_t*)malloc(width * 16);
			if(data){
				img.setPixelType(ONE_BIT_DITHERED);
				img.decodeDither(data, 0);
				log_d("freeing data");
				free(data);
				log_d("data freed");
			} else {
				log_e("%i bytes of memory for dithering could not be allocated", width * 16);
			}
		} else {
			// color decoding
			log_d("decoding image");
			img.setPixelType(RGB565_LITTLE_ENDIAN);
			img.decode(0, 0, 0);
		}
		img.close();
		imgState = IMG_PROCESSED;
	} else {
		log_e("couldn't open %s for processing", newFilePath);
	}
}
