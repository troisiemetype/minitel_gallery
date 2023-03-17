#include "minitel_bt_images.h"
/*
#include "FS.h"
#include "SPI.h"
#include "SD.h"


#include "JPEGDEC.h"
#include "Minitel1B_Hard.h"

#include "bt_main.h"
#include "img.h"
*/

/*
 *	Some notes :
 *	Minitel display has :
 *		40 columns in Videotex mode, and 80 columns in Mixte mode.
 *		24 lines  (plus line 0) in both modes.
 *	G1 set breaks each character into a 2x3 "pixel" matrix.
 *	G1 is usable only in Videotex mode.
 *
 *	Thus the resolution for displaying image is :
 *		80x72 in Videotexmode with G1 set.
 *		40x24 in Videotex mode with characters.
 *		80x24 in Mixte mode with characters.
 *	
 */

/*
 *	Memo : options for File.open() :
 *		"r" 	Open for reading.					Stream at beginning.
 *		"r+" 	As above + writing.
 *		"w" 	Truncate to zero, or create.		Stream at beginning.
 *		"w+" 	As above + reading.
 *		"a"		Open for appending, or create.		Stream at end.
 *		"a+"	Same, plus reading.					Initial Stream for reading at beginning, but output at end.
 */

#define MINITEL_MAX_WIDTH		80
#define MINITEL_MAX_HEIGHT		72

/*
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
*/

//SIMGlass spi = SIMGlass(VSPI);

Minitel minitel(Serial2);

uint16_t speed = 0;

const char *sdMount = "/";
char rawFileName[80];
char newFilePath[60];
char newFileName[40];
char fileType[13];
uint16_t fileSize = 0;

const uint16_t workBfSize = 256;
uint8_t workBf[workBfSize];


// File newFile;
// JPEGDEC img;

struct image_t{
	File file;
	File raw;
	JPEGDEC img;
	imgState_t state;
	uint32_t size;
	uint16_t width;
	uint16_t height;
	uint16_t newSize;
	uint16_t newWidth;
	uint16_t newHeight;
	uint8_t scale;
	float divider;
};

image_t img;

bool useDither = true;

uint32_t timeStamp = 0;

// Get the filename for the file, but more important open a new SD file for write.
void filenameCB(const char* data, uint8_t length){

	uint16_t index = 0;
	for(uint16_t i = 0; i < length; ++i){
		if((i % 2) == 0) continue;
//		log_d("i : %i ; index : %i ; char : %c, 0x%02x", i, index, data[i], data[i]);
		memcpy((char*)&(newFileName[index++]), (const char*)&(data[i]), 1);
	}
	strcpy(newFilePath, "/");
//	strcat(newFilePath, "file.jpg");
	strcat(newFilePath, newFileName);
	log_i("opening %s for write", newFilePath);

	img.file = SD.open(newFilePath, FILE_WRITE, true);
	if(img.file){
		img.state = RECEIVING_IMG;
		timeStamp = millis();
		log_i("file open successfully");
	} else {
		log_e("could not open file");
	}

}

void fileSizeCB(uint16_t size){
	fileSize = size;
}

void fileTypeCB(const char* data, uint8_t length){
	strcpy(fileType, data);
	log_d("file type : %s", fileType);
}

void dataCB(const uint8_t* data, uint16_t length){
	if(img.state == RECEIVING_IMG){
		log_d("writing %i bytes of data to SD", length);
		img.file.write(data, length);
	}
}

void dataEndCB(){
	if(img.state == RECEIVING_IMG){
		log_i("received file in %i seconds", (millis() - timeStamp) / 1000);
		log_d("closing file");
		img.state = IMG_RECEIVED;
		img.file.close();
	}
}

// This is a helper function to write data aat any position in a file.
// File should be opened in a read + write mode
// Buffer could be any size, I believe the bigger the better.
bool insertToFile(File *file, uint32_t pos, const uint8_t* data, uint32_t len){
	if(!file) return 0;
	const uint16_t bufferSize = 256;
	uint8_t bf[bufferSize];
	// File.available() and file.size() are not updated until close.
	// Thus we will use seek() + position() to know current size.
	file->seek(0, SeekEnd);
	size_t size = file->position();
	if(size <= 0) return 0;
	int32_t rPos = size;

	log_d("file is %i bytes long", size);
	log_d("adding %i bytes at the end of the file", len);
	// we start by adding any bytes (as buffer is not initialized) to the end of the file.
	// seek(SeekEnd) may be more appropriate, but I'm not sure how to handle it.
	uint16_t loops = len / bufferSize;
	uint16_t remain = len % bufferSize;

	for(uint16_t i = 0; i < loops; ++i){
		log_d("adding %i bytes, total : %i", bufferSize, (i + 1) * bufferSize);
		file->write(bf, bufferSize);
		file->flush();
	}
	log_d("adding %i remaining bytes", remain);
	file->write(bf, remain);
	file->flush();

	// Getting the new size.
	file->seek(0, SeekEnd);
	size = file->position();

	log_d("now file is %i bytes long", size);

	int16_t bytesToMove = bufferSize;
	// now moving the bytes, -bufferSize- at a time.
	while(bytesToMove == bufferSize){
		rPos -= bufferSize;
		if(rPos <=  (int32_t)pos){
			bytesToMove = rPos + bufferSize - pos;
			rPos = pos;
		}
		log_d("moving %i bytes from %i to %i", bytesToMove, rPos, rPos + len);

		file->seek(rPos, SeekSet);
		file->read(bf, bytesToMove);
		file->seek(rPos + len, SeekSet);
		file->write(bf, bytesToMove);
		file->flush();

	}

	// finally everything has been moved, we write our bytes to desired position.
	file->seek(pos, SeekSet);
	file->write(data, len);

	return 1;
}

void setup(){

	Serial.begin(115200);

	log_i("starting");

    // begin(uint8_t ssPin=SS, SIMGlass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5, bool format_if_empty=false);
//	bool sdStarted = SD.begin(SS, SPI, 40000000L, "/");
	bool sdStarted = SD.begin();

	if(!sdStarted){
		log_e("failed to init SD card");
		return;
	}
	log_i("SD card init");

	memset(&img, 0, sizeof(image_t));

	setFilenameCB(filenameCB);
	setFileTypeCB(fileTypeCB);
	setDataCB(dataCB);
	setDataEndCB(dataEndCB);

	init_bt();

//	Serial.printf("speed : %i\n", speed);

	speed = minitel.searchSpeed();

	speed = 4800;

	minitel.changeSpeed(speed);

//	minitel.newScreen();
//	minitel.println("starting.");

	delay(2000);
	minitel.newScreen();
}

void img_logRawBytes(const uint8_t *data, uint16_t length){
	for(int16_t i = 0; i < length; ++i){
		if(((i) % 16) == 0) printf("\n %4i ", i);
		printf("0x%02x ", data[i]);
	}
	printf("\n");

}

void* img_openCB(const char *filename, int32_t *size){
	log_d("CB : open image");
	img.file = SD.open(filename);
	if(img.file){
		*size = img.file.size();
		img.state = PROCESSING_IMG;
		log_d("image opened");
	} else {
		// What do we do when the picture cannot be opened ?
		log_e("couln't open image");
	}

	return &img.file;
}

void img_closeCB(void *handle){
	log_d("CB : close image");
	if(img.file) img.file.close();
}

int32_t img_readCB(JPEGFILE *file, uint8_t *buf, int32_t len){
	log_d("CB : read %i bytes of image", len);

	if(!img.file){
		log_e(" file error");
		return 0;
	}
	return img.file.read(buf, len);
}

int32_t img_seekCB(JPEGFILE *file, int32_t pos){
	log_d("CB : seek image at %i", pos);
	if(!img.file){
		log_e(" file error");
		return 0;
	}
	return img.file.seek(pos);
}

int img_decodeCB(JPEGDRAW *draw){
	log_d("decoding block");
	log_d("position : %i, %i", draw->x, draw->y);
	log_d("size : %i x %i", draw->iWidth, draw->iHeight);
	log_d("bit depth : %i", draw->iBpp);
	uint16_t size = draw->iWidth * draw->iHeight;
//	log_d("%i 16-bit words", size);
	img.state = IMG_PROCESSED;

//	img_logRawBytes((uint8_t*)draw->pPixels, size);

	// here we compute the data from the image
	// We need to do several things.
	// the image is already in gray scale
	// We need to shrink it by the img.divider factor.
	// For now we floor the image dividor factor to easeup computing.
//	img.divider = floor(img.divider);

	for(uint16_t i = 0; i < draw->iHeight; ++i){
		uint32_t index = (draw->y + i) * img.newWidth + draw->x * draw->iWidth;
		bool ret = img.raw.seek(index, SeekSet);
		if(ret){
			uint16_t limit = draw->iWidth;
			if((limit * (draw->x + 1)) > img.newWidth) limit = img.newWidth - draw->x * draw->iWidth;
			log_d("write %i bytes of data to %i", limit, index);
			img.raw.write((uint8_t*)&(draw->pPixels[i * draw->iWidth / sizeof(uint16_t)]), limit);
//			img.raw.flush();
		} else {
			log_d("writing to %i impossible", index);
		}
	}

	return 1;
}

void img_decode(const char* file){

	// open(fileName, openCB, closeCB, readCB, seekCB, drawCB)
	int ret = img.img.open(file, img_openCB, img_closeCB, img_readCB, img_seekCB, img_decodeCB);
	if(ret){
		img.width = img.img.getWidth();
		img.height = img.img.getHeight();
		img.size = img.width * img.height;
		log_i("new image %i x %i", img.width, img.height);

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

		img.divider = 1;

		img.scale = 0;

		widthDivider = (float)img.width / MINITEL_MAX_WIDTH;
		heightDivider = (float)img.height / MINITEL_MAX_HEIGHT;

		log_d("width divider : %.2f", widthDivider);
		log_d("height divider : %.2f", heightDivider);

		if(widthDivider > heightDivider){
			log_d("keeping height");
			img.divider = heightDivider;
		} else {
			log_d("keeping width");
			img.divider = widthDivider;
		}

		img.newWidth = img.width;
		img.newHeight = img.height;

		if(img.divider < 1) img.divider = 1;

		if(img.divider > 8){
			img.scale |= JPEG_SCALE_EIGHTH;
			img.divider /= 8;
			img.newWidth /= 8;
			img.newHeight /= 8;
		} else if( img.divider > 4){
			img.scale |= JPEG_SCALE_QUARTER;
			img.divider /= 4;
			img.newWidth /= 4;
			img.newHeight /= 4;
		} else if(img.divider > 2){
			img.scale |= JPEG_SCALE_HALF;
			img.divider /= 2;
			img.newWidth /= 2;
			img.newHeight /= 2;
		}
/*
		img.newWidth /= img.divider;
		img.newHeight /= img.divider;
*/
		img.newSize = img.newWidth * img.newHeight;

		log_i("image new size : %ix%i, with %.2f divider and %i scaling", img.newWidth, img.newHeight, img.divider, img.scale);

		img.raw = SD.open("/img.raw", "w", true);
		if(!img.raw){			
			log_e("could not open %s for write", "/img.raw");
			img.state = IMG_ABORTED;
			img.img.close();
			return;
		}

		memset(workBf, 0, workBfSize);
		uint32_t loops = img.newSize / workBfSize;
		uint16_t remain = img.newSize % workBfSize;
		for (uint32_t i = 0; i < loops; ++i){
			img.raw.write(workBf, workBfSize);
			img.raw.flush();
		}
		img.raw.write(workBf, remain);
		img.raw.flush();

		useDither = false;
		if(useDither){
			// Dither decoding
			log_d("decoding and dithering image");
			const uint8_t dataSize = img.width * 16;
//			uint8_t *data = (uint8_t*)malloc(dataSize);
			uint8_t data[dataSize];
			if(data){
				img.img.setPixelType(ONE_BIT_DITHERED);
				img.img.decodeDither(data, 0);
/*
				log_d("freeing data");
				free(data);
				log_d("data freed");
*/			} else {
				log_e("%i bytes of memory for dithering could not be allocated", dataSize);
			}
		} else {
			// color decoding
			log_d("decoding image");
			img.img.setPixelType(EIGHT_BIT_GRAYSCALE);
			img.img.decode(0, 0, img.scale);
		}
		img.img.close();
		img.raw.close();
		img.state = IMG_PROCESSED;
	} else {
		log_e("couldn't open %s for processing", newFilePath);
		img.state = IMG_ABORTED;
	}
}

void loop(){

	if(img.state == IMG_RECEIVED){
		uint32_t then = millis();
		img_decode(newFilePath);
		img.state = IDLE;
		log_i("decoded in %is", (millis() - then) / 1000);

		img.raw = SD.open("/img.raw", "r");
		if(img.raw){
			minitel.graphicMode();
			minitel.attributs(DEBUT_LIGNAGE);
			log_d("drawing to screen");

			for(uint16_t j = 0; j < 24; ++j){
				for(uint16_t i = 0; i < img.newWidth / 2; ++i){
					if(i > 40){
						img.raw.read();
						img.raw.read();
						continue;
					}
					uint16_t val = img.raw.read() + img.raw.read();	
					val /= 2;
					if(val > 127) minitel.graphic(0b111111);
					else minitel.graphic(0b000000);
				}
			}

		} else {
			log_d("could not open /img.raw");
		}
	}
}