#include "FS.h"
#include "SPI.h"
#include "SD.h"


#include "Minitel1B_Hard.h"
#include "bt_main.h"

/*
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
*/

//SPIClass spi = SPIClass(VSPI);

Minitel minitel(Serial2);

uint16_t speed = 0;

const char *sdMount = "/";
char rawFileName[80];
char newFilePath[60];
char newFileName[40];
char fileType[13];
uint16_t fileSize = 0;

File newFile = File();

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
	log_d("opening %s for write", newFilePath);

	newFile = SD.open(newFilePath, FILE_WRITE, true);
	if(newFile){
		log_d("file open successfully");
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
	if(newFile){
		log_d("writing %i bytes of data to SD", length);
		newFile.write(data, length);
	}
}

void dataEndCB(){
	if(newFile){
		log_d("closing file");
		newFile.close();
	}
}

void setup(){

	Serial.begin(115200);

//	speed = minitel.searchSpeed();

	log_i("starting");

    // begin(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5, bool format_if_empty=false);
//	bool sdStarted = SD.begin(SS, SPI, 40000000L, "/");
	bool sdStarted = SD.begin();

	if(!sdStarted){
		log_e("failed to init SD card");
		return;
	}
	log_i("SD card init");

	setFilenameCB(filenameCB);
	setFileTypeCB(fileTypeCB);
	setDataCB(dataCB);
	setDataEndCB(dataEndCB);
	
	init_bt();

//	Serial.printf("speed : %i\n", speed);

//	minitel.changeSpeed(speed);

//	minitel.newScreen();
//	minitel.println("starting.");

	delay(2000);
//	minitel.newScreen();
}

void loop(){

}