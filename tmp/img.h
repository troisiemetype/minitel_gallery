#ifndef MINITEL_IMG_H
#define MINITEL_IMG_H

#include "minitel_bt_images.h"

void img_logRawBytes(const uint8_t *data, uint16_t length);

void* img_openCB(const char *filename, int32_t *size);
void img_closeCB(void *handle);
int32_t img_readCB(JPEGFILE *file, uint8_t *buf, int32_t len);
int32_t img_seekCB(JPEGFILE *file, int32_t pos);
int img_decodeCB(JPEGDRAW *draw);

void img_decode(const char*, imgState_t*);


#endif