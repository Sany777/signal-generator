#include "generator.h"


#include "main.h"
#include <inttypes.h>
#include "ssd1306.h"
#include "si5351.h"
#include "ssd1306_fonts.h"

#include "stdio.h"

static const char * strength_verb[] = {"2mA", "4mA", "6mA", "8mA", "OFF"};
static char buf[100];


void flash_read_data(void* dst, size_t len)
{
	uint32_t addr = FLASH_PAGE_ADDR;
    uint32_t* word_data = (uint32_t*)dst;
    size_t words_num = (len + 3) / 4;

    for (size_t i = 0; i < words_num; i++) {
        word_data[i] = *(volatile uint32_t*)(addr + i * 4);
    }
}

void flash_write_data(const void* src, size_t len)
{
	uint32_t addr = FLASH_PAGE_ADDR;
    HAL_StatusTypeDef status;
    const uint32_t* word_data = (const uint32_t*)src;
    size_t words_num = (len + 3) / 4;

    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef eraseInitStruct = { 0 };
    uint32_t pageError = 0;
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.PageAddress = FLASH_PAGE_ADDR;
    eraseInitStruct.NbPages = 1;

    if(HAL_FLASHEx_Erase(&eraseInitStruct, &pageError) == HAL_OK){
		for (size_t i = 0; i < words_num; i++) {
			status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i * 4, word_data[i]);
			if (status != HAL_OK) {
				break;
			}
		}
    }
    HAL_FLASH_Lock();
}

void display_help()
{
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("1 click: set cursor", Font_6x8, White);
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("2 click: set CH", Font_6x8, White);
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString("4 click: fix freq.", Font_6x8, White);
	ssd1306_UpdateScreen();
}

void display_correction_data(int32_t corection_val, uint32_t phase, uint32_t cursor,  bool out_enable)
{
	ssd1306_Fill(Black);
	ssd1306_SetCursor(25, 0);
	ssd1306_WriteString(out_enable ? "OUT ON" : "OUT OFF", Font_7x10, White);
	ssd1306_SetCursor(10, 10);
	snprintf(buf, sizeof(buf), "freq.:  %+d", corection_val);
	ssd1306_WriteString(buf, Font_7x10, cursor ? Black : White);
	ssd1306_SetCursor(10, 20);
	snprintf(buf, sizeof(buf), "phase.:  %u", phase);
	ssd1306_WriteString(buf, Font_7x10, cursor ? White : Black);
	ssd1306_UpdateScreen();
}


void display_ch_data(channel_settings *channels, uint32_t cur_ch_ind, uint32_t cursor, bool out_enable)
{
	channel_settings *ptr = channels;
	const channel_settings *cur_ch = channels + cur_ch_ind;
	const uint32_t freq = cur_ch->freq;
	ssd1306_Fill(Black);
	ssd1306_SetCursor(50, 0);
	snprintf(buf, sizeof(buf), "CH %u  ", cur_ch_ind+1);
	ssd1306_WriteString(buf, Font_7x10, White);
	ssd1306_WriteString(out_enable ? "On" : "Off", Font_7x10, White);

	for(int i=0, c='1'; i<CH_NUM; ++i, ++ptr){
		if(ptr->driveStrength != SI5351_DRIVE_STRENGTH_OFF){
			ssd1306_DrawCircle(3, i*10 + 5, 2, White);
		}
		ssd1306_SetCursor(10, i*10);
		ssd1306_WriteChar(c++, Font_6x8, cur_ch_ind == i ? Black : White);
	}

	ssd1306_SetCursor(20, 15);
	snprintf(buf, sizeof(buf), "%03ld", freq / 1000000);
	ssd1306_WriteString(buf, Font_7x10, cursor == 0 ? Black : White);
	ssd1306_WriteChar('.', Font_6x8, White);
	snprintf(buf, sizeof(buf), "%03ld", (freq / 1000) % 1000);
	ssd1306_WriteString(buf, Font_7x10, cursor == 1 ? Black : White);
	ssd1306_WriteChar('.', Font_6x8, White);
	snprintf(buf, sizeof(buf), "%03ld ", freq % 1000);
	ssd1306_WriteString(buf, Font_7x10, cursor == 2 ? Black : White);
	ssd1306_WriteString(strength_verb[cur_ch->driveStrength], Font_7x10, cursor == 3 ? Black : White);
	ssd1306_UpdateScreen();
}

void set_correction_val(settings_t *settings, uint32_t cursor, int32_t enc_value)
{
  if(cursor){
	  settings->corection += enc_value;
	  si5351_set_correction(settings->corection);
  } else {
	  settings->phase += enc_value;
	  if(settings->phase > 180){
		  settings->phase = 0;
	  } else if(settings->phase < 0){
		  settings->phase = 180;
	  }
  }
}

void set_ch_val(channel_settings *channel, uint32_t cursor, int32_t enc_value)
{
	int cor_val;
	switch(cursor){
		case CURSOR_CHANGING_FREQUENCY_HZ:
			{
				cor_val = channel->freq % 1000;
				channel->freq -= cor_val;
				cor_val += enc_value;
				if(cor_val > 999){
					cor_val = 0;
				} else if(cor_val < 0){
					cor_val = 999;
				}
				channel->freq += cor_val;
				break;
			}
		case CURSOR_CHANGING_FREQUENCY_kHZ:
			{
				cor_val = (channel->freq /1000) % 1000;
				channel->freq -= cor_val * 1000;
				cor_val += enc_value;
				if(cor_val > 999){
					cor_val = 0;
				} else if(cor_val < 0){
					cor_val = 999;
				}
				channel->freq += cor_val * 1000;
				break;
			}
		case CURSOR_CHANGING_FREQUENCY_MHZ:
			{
				cor_val = channel->freq /1000000;
				channel->freq -= cor_val * 1000000;
				cor_val += enc_value;
				if(cor_val > 160){
					cor_val = 0;
				} else if(cor_val < 0){
					cor_val = 160;
				}
				channel->freq += cor_val * 1000000;
				break;
			}
		case CURSOR_CHANGING_POWER:
			{
				channel->driveStrength += enc_value > 0 ? 1 : -1;
				break;
			}
		default : break;
	}


	if(cursor == CURSOR_CHANGING_POWER){
		if(channel->driveStrength < SI5351_DRIVE_STRENGTH_2MA){
			channel->driveStrength = SI5351_DRIVE_STRENGTH_OFF;
		} else if(channel->driveStrength > SI5351_DRIVE_STRENGTH_OFF){
			channel->driveStrength = SI5351_DRIVE_STRENGTH_2MA;
		}
		if(channel->driveStrength != SI5351_DRIVE_STRENGTH_OFF){
			if(channel->freq > MAX_FREQ){
				channel->freq = MAX_FREQ;
			} else if(channel->freq < MIN_FREQ){
				channel->freq = MIN_FREQ;
			}
		}
	}
}


int get_but_state()
{
	int ret_val = BUT_STATE_RELEASE;
	int timeout, press_time, debounce_count;
	bool but_state, last_state;
	if(HAL_GPIO_ReadPin(gpio_key_in_GPIO_Port, gpio_key_in_Pin) == GPIO_PIN_RESET){
		timeout = press_time = 0;
		last_state = true;
		debounce_count = 0;
		ret_val = BUT_STATE_PRESSED;
		do{
			but_state = HAL_GPIO_ReadPin(gpio_key_in_GPIO_Port, gpio_key_in_Pin) == GPIO_PIN_RESET;

			if(last_state != but_state){
				if(debounce_count < 50){
					debounce_count += 10;
				} else {
					debounce_count = 0;
					last_state = but_state;
					if(but_state){
						ret_val += 1;
					}
				}
			} else if (debounce_count){
				debounce_count -= 10;
			}
			if(last_state){
				if(press_time>700){
					ret_val = BUT_STATE_LONG_PRESSED;
					break;
				}
				press_time += 10;
			} else if(press_time){
				press_time -= 10;
			}

			HAL_Delay(10);
			timeout += 10;
		}while(timeout < 1000);
	}
	return ret_val;
}

void fix_ch_data(channel_settings *channels)
{
	const channel_settings *end = channels + CH_NUM;
	while(channels < end){
		if(channels->driveStrength < SI5351_DRIVE_STRENGTH_2MA
					|| channels->driveStrength > SI5351_DRIVE_STRENGTH_OFF){
			channels->driveStrength = SI5351_DRIVE_STRENGTH_OFF;
		}
		if(channels->freq < 8000 || channels->freq > 160000000){
			channels->freq = 8000;
		}
		++channels;
	}
}
