	const char* text = "This is a new text for testing seek function, and see how the seek options chnge things when writing to the file.";
	const char* textSSet = "SeekSet";
	const char* textSCur = "SeekCur";
	const char* textSEnd = "SeekEnd";

	File file = SD.open("/test.txt", FILE_WRITE, true);
	if(!file){
		log_e("could not open file");
		return;
	}
/*
	for(uint8_t i = 0; i < 32; ++i){
		file.write(i);
	}

	if(file.seek(8, SeekSet)){
		for(uint8_t i = 0; i < 8; ++i){
			file.write(i);
		}
	}

	if(file.seek(2, SeekCur)){
		file.write(0);
	}

	if(file.seek(2, SeekEnd)){
		file.write(255);
	}
*/
	
	file.write((uint8_t*)text, strlen(text));

	// SeekSet position from the start and overwrites existing data.
	if(file.seek(4, SeekSet)){
		file.write((uint8_t*)textSSet, strlen(textSSet));
	}
	// SeekCur position from current and overwrites existing data.
	if(file.seek(2, SeekCur)){
		file.write((uint8_t*)textSCur, strlen(textSCur));
	}

	// SeekEnd position after the end of the file, pads with 0 data and append data.
	if(file.seek(12, SeekEnd)){
		file.write((uint8_t*)textSEnd, strlen(textSEnd));
	}

	file.close();


	const char* text = "This is a new text for testing seek function, and see how the seek options changes things when writing to the file.";
	const char* textToInsert = "inserting data in the middle of a file, using buffer to re-write it from the end, ";
	File file = SD.open("/test.txt", "w+", true);
	if(!file){
		log_e("could not open file");
		return;
	}

	uint8_t insertIndex = 46;

	file.write((uint8_t*)text, strlen(text));

	log_i("inserting \"%s\" at pos %i in following text :", textToInsert, insertIndex);
	log_i("%s", text);

	if(insertToFile(&file, insertIndex, (const uint8_t*)textToInsert, strlen(textToInsert))){
		file.seek(0, SeekEnd);
		uint16_t toRead = file.position();
		file.seek(0, SeekSet);
		log_i("now reading %i bytes", toRead);
		for(uint16_t i = 0; i < toRead; ++i){
			printf("%c", file.read());
		}
		printf("\n");
	}


	file.close();