/*
 * V2X_time.c
 *
 * Created: 3/2/2016 3:24:15 PM
 *  Author: jbanks2
 */ 

#include "V2X.h"

Bool alarm_is_set = false;
Bool time_is_set = false; 
long time_was_set = 0; //marks the last time the clock was adjusted
long alarm_will_expire = 0;
int time_zone = 0;
Bool dst = 0;

struct calendar_date date_s = {
	.second = 12,
	.minute = 1,
	.hour = 22,
	.date = 8,
	.month = 2,
	.year = 1985
};

void time_init(void) {
	rtc_init();
	rtc_set_time(time_seed);
	rtc_set_callback(time_alarm_event);
	time_zone = nvm_eeprom_read_byte(EE_timezone) - TZ_OFFSET;
	dst = nvm_eeprom_read_byte(EE_dst);
}

void time_set(long time) {
	long delta_time = 0;
	if (alarm_is_set) { //if alarm set, get offset to alarm time
		delta_time = alarm_will_expire - rtc_get_time();
	}
	rtc_set_time(time);
	time_was_set = time;
	time_is_set = true;
	
	if (alarm_is_set) { 
		time_alarm_set_relative(delta_time); 
	}
}

void time_dst_set(Bool set) {
	if (set) {
		dst = true;
	} else {
		dst = false;
	}
	nvm_eeprom_write_byte(EE_dst , dst);
}

Bool time_dst_get (void) {
	return dst;
}

long time_get(void) {
	time_24hr_check();
	return  rtc_get_time();
}

long time_alarm_get(void) {
	return  alarm_will_expire;
}

void time_alarm_set(long time) {
	alarm_is_set = true;
	alarm_will_expire = time;
	rtc_set_alarm(time);
}

void time_alarm_set_relative(long offset) {
	alarm_will_expire = rtc_get_time() + offset;
	time_alarm_set(alarm_will_expire);
}

void time_alarm_event(void) {
	alarm_is_set = false;
	menu_send_CTL();
	usb_tx_string_P(PSTR("ALARM"));
	menu_send_n_st();
	//respond to alarm
	PWR_host_start();
	//set flag that time alarm caused wakeup
}

Bool time_alarm_active(void) {
	return alarm_is_set;
}

Bool time_is_current(void) {
	time_24hr_check();
	return time_is_set;
}

void time_24hr_check (void){
	if (rtc_get_time() - time_was_set > RTC_VALIDITY_PERIOD) {time_is_set = false;}
}

void time_zone_set (int zone) {
	nvm_eeprom_write_byte(EE_timezone , zone + TZ_OFFSET);
	time_zone = zone;
}

int time_zone_get (void) {
	return (time_zone);
}

void time_print_human_readable (void) {
	calendar_timestamp_to_date_tz(rtc_get_time(),time_zone + dst,0,&date_s);
	menu_print_int(date_s.month+1);
	usb_tx_string_P(PSTR("/"));
	menu_print_int(date_s.date+1);
	usb_tx_string_P(PSTR("/"));
	menu_print_int(date_s.year);
	usb_tx_string_P(PSTR(" "));
	menu_print_int(date_s.hour);
	usb_tx_string_P(PSTR(":"));
	if (date_s.minute < 10) {usb_tx_string_P(PSTR("0"));} //add a 0 if below 10
	menu_print_int(date_s.minute);
	usb_tx_string_P(PSTR(":"));
	if (date_s.second < 10) {usb_tx_string_P(PSTR("0"));} //add a 0 if below 10
	menu_print_int(date_s.second);
}

void time_set_by_strings (char * date, char * time) {
	char bufr[3] = "\0";
	for (int i = 0; i < 2; i++) {bufr[i] = date[i];}
	date_s.date = atoi(bufr)-1;
	for (int i = 0; i < 2; i++) {bufr[i] = date[i+2];}
	date_s.month = atoi(bufr)-1;
	for (int i = 0; i < 2; i++) {bufr[i] = date[i+4];}
	date_s.year = atoi(bufr) + 2000;
	for (int i = 0; i < 2; i++) {bufr[i] = time[i];}
	date_s.hour = atoi(bufr);
	for (int i = 0; i < 2; i++) {bufr[i] = time[i+2];}
	date_s.minute = atoi(bufr);
	for (int i = 0; i < 2; i++) {bufr[i] = time[i+4];}
	date_s.second = atoi(bufr);

	long new_time = calendar_date_to_timestamp_tz(&date_s, 0, 0); 	// GPS time handled as Zulu time, no offset while setting
	time_set(new_time);

}


