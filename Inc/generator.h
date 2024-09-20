#ifndef GENERATOR_H_
#define GENERATOR_H_



#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"

#define MAX_FREQ 				160000000
#define MIN_FREQ 				8000
#define FLASH_PAGE_ADDR  		0x08007C00
#define BUT_LONG_PRESS_TIME 	1000
#define BUT_PRESS_TIME 			200



enum{
	CH_1,
	CH_2,
	CH_3,
	CH_NUM,
};

typedef enum {
	CURSOR_CHANGING_FREQUENCY_MHZ,
	CURSOR_CHANGING_FREQUENCY_kHZ,
	CURSOR_CHANGING_FREQUENCY_HZ,
	CURSOR_CHANGING_POWER,
	CURSOR_CHANGING_MAX,
} CursorChanging_t;


typedef struct{
	int32_t freq;
	int32_t driveStrength;
}channel_settings;

typedef struct{
	  channel_settings channels[CH_NUM];
	  int32_t corection;
	  int32_t phase;
}settings_t;

enum ButState{
	BUT_STATE_LONG_PRESSED 	= -1,
	BUT_STATE_RELEASE 		= 0,
	BUT_STATE_PRESSED 		= 1,
	BUT_DOUBLE_PRESSED 		= 2,
	BUT_FOUR_PRESSED 		= 4,
};


void fix_ch_data(channel_settings *channels);
int get_but_state();
void display_help();
void display_correction_data(int32_t corection_val, uint32_t phase, uint32_t cursor,  bool out_enable);
void display_ch_data(channel_settings *channels, uint32_t cur_ch, uint32_t cursor, bool out_enable);
void flash_read_data(void* dst, size_t len);
void flash_write_data(const void* src, size_t len);
void set_ch_val(channel_settings *channel, uint32_t cursor, int32_t enc_value);
void set_correction_val(settings_t *settings, uint32_t cursor, int32_t enc_value);

#ifdef __cplusplus
}
#endif

#endif /* GENERATOR_H_ */
