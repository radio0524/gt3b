/*
    menu - handle popup menus
    Copyright (C) 2011 Pavel Semerad

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <string.h>
#include "menu.h"
#include "config.h"
#include "calc.h"
#include "timer.h"
#include "ppm.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"






// ********************* TRIMS ****************************


// mapping of keys to trims
const u16 et_buttons[][2] = {
    { BTN_TRIM_LEFT,  BTN_TRIM_RIGHT },
    { BTN_TRIM_FWD,   BTN_TRIM_BCK },
    { BTN_TRIM_CH3_L, BTN_TRIM_CH3_R },
    { BTN_DR_L,       BTN_DR_R },
};
#define ET_BUTTONS_SIZE  (sizeof(et_buttons) / sizeof(u16) / 2)
#define ETB_L(id)  et_buttons[id][0]
#define ETB_R(id)  et_buttons[id][1]




// functions assignable to trims
typedef struct {
    u8 idx;		// will be showed sorted by this
    u8 *name;		// showed identification
    u8 menu;		// which menu item(s) show
    u8 flags;		// bits below
    u8 channel;		// show this channel number
    void *aval;		// address of variable
    s16 min, max, reset; // limits of variable
    u8 rot_fast_step;	// step for fast encoder rotate
    u8 *labels;		// labels for trims
    void (*long_func)(s16 *aval, u16 btn_l, u16 btn_r);  // function for special long-press handling
} et_functions_s;
#define EF_NONE		0
#define EF_RIGHT	0b00000001
#define EF_LEFT		0b00000010
#define EF_PERCENT	0b00000100
#define EF_NOCHANNEL	0b01000000
#define EF_BLINK	0b10000000

static const et_functions_s et_functions[] = {
    { 0, "OFF", 0, EF_NONE, 0, NULL, 0, 0, 0, 0, NULL, NULL },
    { 1, "TR1", LM_TRIM, EF_NONE, 1, &cm.trim[0], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "LNR", NULL },
    { 2, "TR2", LM_TRIM, EF_NONE, 2, &cm.trim[1], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "FNB", NULL },
    { 7, "DRS", LM_DR, EF_PERCENT, 1, &cm.dr_steering, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL },
    { 8, "DRF", LM_DR, EF_PERCENT | EF_LEFT, 2, &cm.dr_forward, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL },
    { 9, "DRB", LM_DR, EF_PERCENT | EF_RIGHT, 2, &cm.dr_back, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL },
    { 10, "EXS", LM_EXP, EF_PERCENT, 1, &cm.expo_steering, -EXPO_MAX, EXPO_MAX,
      0, EXPO_FAST, NULL, NULL },
    { 11, "EXF", LM_EXP, EF_PERCENT | EF_LEFT, 2, &cm.expo_forward,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL, NULL },
    { 12, "EXB", LM_EXP, EF_PERCENT | EF_RIGHT, 2, &cm.expo_back,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL, NULL },
    { 13, "CH3", 0, EF_NONE, 3, &menu_channel3, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 19, "ST1", LM_TRIM, EF_BLINK, 1, &cm.subtrim[0], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
    { 20, "ST2", LM_TRIM, EF_BLINK, 2, &cm.subtrim[1], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
    { 21, "ST3", LM_TRIM, EF_BLINK, 3, &cm.subtrim[2], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#if MAX_CHANNELS >= 4
    { 14, "CH4", 0, EF_NONE, 4, &menu_channel4, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 22, "ST4", LM_TRIM, EF_BLINK, 4, &cm.subtrim[3], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#if MAX_CHANNELS >= 5
    { 15, "CH5", 0, EF_NONE, 5, &menu_channel5, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 23, "ST5", LM_TRIM, EF_BLINK, 5, &cm.subtrim[4], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#if MAX_CHANNELS >= 6
    { 16, "CH6", 0, EF_NONE, 6, &menu_channel6, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 24, "ST6", LM_TRIM, EF_BLINK, 6, &cm.subtrim[5], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#if MAX_CHANNELS >= 7
    { 17, "CH7", 0, EF_NONE, 7, &menu_channel7, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 25, "ST7", LM_TRIM, EF_BLINK, 7, &cm.subtrim[6], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#if MAX_CHANNELS >= 8
    { 18, "CH8", 0, EF_NONE, 8, &menu_channel8, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL },
    { 26, "ST8", LM_TRIM, EF_BLINK, 8, &cm.subtrim[7], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL },
#endif
#endif
#endif
#endif
#endif
    { 27, "4WS", LM_EPO, EF_BLINK | EF_PERCENT | EF_NOCHANNEL, 4,
      &menu_4WS_mix, -100, 100, 0, MIX_FAST, NULL, NULL },
    { 28, "DIG", LM_EPO, EF_BLINK | EF_PERCENT | EF_NOCHANNEL, L7_D,
      &menu_DIG_mix, -100, 100, 0, MIX_FAST, NULL, NULL },
    { 29, "SST", LM_DR, EF_BLINK | EF_LEFT | EF_PERCENT, 1, &cm.stspd_turn,
      1, 100, 100, SPEED_FAST, NULL, NULL },
    { 30, "SSR", LM_DR, EF_BLINK | EF_RIGHT | EF_PERCENT, 1, &cm.stspd_return,
      1, 100, 100, SPEED_FAST, NULL, NULL },
};
#define ET_FUNCTIONS_SIZE  (sizeof(et_functions) / sizeof(et_functions_s))


// return name of given line
u8 *menu_et_function_name(u8 n) {
    return et_functions[n].name;
}

// return sort idx of given line
s8 menu_et_function_idx(u8 n) {
    if (n >= ET_FUNCTIONS_SIZE)  return -1;
    return et_functions[n].idx;
}

// return if given function has special long-press handling
u8 menu_et_function_long_special(u8 n) {
    return (u8)(et_functions[n].long_func ? 1 : 0);
}



const u8 steps_map[STEPS_MAP_SIZE] = {
    1, 2, 5, 10, 20, 30, 40, 50, 67, 100, 200,
};





// temporary show popup value (trim, subtrim, dualrate, ...)
// if another key pressed, return
#define AVAL(x)  *(s8 *)etf->aval = (s8)(x)
static u8 menu_popup_et(u8 trim_id) {
    u16 delay_time;
    s16 val;
    u8  step;
    u16 buttons_state_last;
    u16 btn_l = ETB_L(trim_id);
    u16 btn_r = ETB_R(trim_id);
    u16 btn_lr = btn_l | btn_r;
    config_et_map_s *etm = &ck.et_map[trim_id];
    et_functions_s *etf = &et_functions[etm->function];

    // if keys are momentary, show nothing, but set value
    if (etm->buttons == ETB_MOMENTARY) {
	if (btns(btn_l)) {
	    // left
	    AVAL(etm->reverse ? etf->max : etf->min);
	}
	else if (btns(btn_r)) {
	    // right
	    AVAL(etm->reverse ? etf->min : etf->max);
	}
	else {
	    // center
	    AVAL(etf->reset);
	}
	return 0;
    }

    // return when key was not pressed
    if (!btn(btn_lr))	 return 0;

    // remember buttons state
    buttons_state_last = buttons_state & ~btn_lr;

    // convert steps
    step = steps_map[etm->step];

    // read value
    if (etf->min >= 0)  val = *(u8 *)etf->aval;	// *aval is unsigned
    else                val = *(s8 *)etf->aval;	// *aval is signed

    // show MENU and CHANNEL
    lcd_menu(etf->menu);
    if (etf->flags & EF_BLINK) lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, (u8)(etf->flags & EF_PERCENT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_LEFT, (u8)(etf->flags & EF_LEFT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_RIGHT, (u8)(etf->flags & EF_RIGHT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_CHANNEL, (u8)(etf->flags & EF_NOCHANNEL ? LS_OFF : LS_ON));
    lcd_7seg(etf->channel);

    while (1) {
	u8  val_set_to_reset = 0;

	// check value left/right
	if (btnl_all(btn_lr)) {
	    key_beep();
	    if (etf->long_func && etm->buttons == ETB_SPECIAL)
		// special handling
		etf->long_func(&val, btn_l, btn_r);
	    else
		// reset to given reset value
		val = etf->reset;
	    AVAL(val);
	    btnr(btn_lr);
	}
	else if (btn(btn_lr)) {
	    if (!btns_all(btn_lr)) {
		key_beep();
		if (etf->long_func && etm->buttons == ETB_SPECIAL &&
		    btnl(btn_lr))
		    // special handling
		    etf->long_func(&val, btn_l, btn_r);
		else {
		    if ((etm->buttons == ETB_LONG_RESET ||
			etm->buttons == ETB_LONG_ENDVAL) && btnl(btn_lr)) {
			// handle long key press
			if (etm->buttons == ETB_LONG_RESET)
			    val = etf->reset;
			else {
			    // set side value
			    if ((u8)(btn(btn_l) ? 1 : 0) ^ etm->reverse)
				val = etf->min;
			    else
				val = etf->max;
			}
		    }
		    else {
			// handle short key press
			if ((u8)(btn(btn_l) ? 1 : 0) ^ etm->reverse) {
			    val -= step;
			    if (val < etf->min)  val = etf->min;
			    if (etm->opposite_reset &&
				val > etf->reset)  val = etf->reset;
			}
			else {
			    val += step;
			    if (val > etf->max)  val = etf->max;
			    if (etm->opposite_reset &&
				val < etf->reset)  val = etf->reset;
			}
			if (val == etf->reset)  val_set_to_reset = 1;
		    }
		}
		AVAL(val);
		btnr(btn_lr);
	    }
	    else btnr_nolong(btn_lr);  // keep long-presses for testing-both
	}
	else if (btn(BTN_ROT_ALL)) {
	    s16 val2 = val;
	    val = menu_change_val(val, etf->min, etf->max, etf->rot_fast_step,
	                          0);
	    // if encoder skipped reset value, set it to reset value
	    if (val2 < etf->reset && val > etf->reset ||
	        val2 > etf->reset && val < etf->reset)  val = etf->reset;
	    if (val == etf->reset)  val_set_to_reset = 1;
	    AVAL(val);
	}
	btnr(BTN_ROT_ALL);

	// longer beep at value reset value
	if (val_set_to_reset)  buzzer_on(20, 0, 1);

	// if another button was pressed, leave this screen
	if (buttons)  break;
	if ((buttons_state & ~btn_lr) != buttons_state_last)  break;

	// show current value
	if (etf->labels)	lcd_char_num2_lbl((s8)val, etf->labels);
	else			lcd_char_num3(val);
	lcd_update();

	// if reset value was reached, ignore rotate/btn_lr for some time
	delay_time = POPUP_DELAY * 200;
	if (val_set_to_reset) {
	    u8 delay = RESET_VALUE_DELAY;
	    while (delay && !(buttons & ~(btn_lr | BTN_ROT_ALL)) &&
		   ((buttons_state & ~btn_lr) == buttons_state_last))
		delay = (u8)delay_menu(delay);
	    btnr(BTN_ROT_ALL | btn_lr);
	    delay_time -= RESET_VALUE_DELAY;
	}

	// sleep 5s, and if no button was changed during, end this screen
	while (delay_time && !buttons &&
	       ((buttons_state & ~btn_lr) == buttons_state_last))
	    delay_time = delay_menu(delay_time);

	if (!buttons)  break;  // timeouted without button press
    }

    btnr(btn_lr);  // reset also long values

    // set MENU off
    lcd_menu(0);

    // save model config
    config_model_save();
    return 1;
}





// check trims and dualrate keys, invoke popup menu
// return 1 when popup was activated
u8 menu_electronic_trims(void) {
    u8 i;

    // for each trim, call function
    for (i = 0; i < ET_BUTTONS_SIZE; i++) {
	if (!ck.et_map[i].is_trim)  continue;  // trim is off
	if (menu_popup_et(i))  return 1;
    }

    return 0;
}






// ************************* KEYS *************************


// mapping of keys to no-trims
static const u16 key_buttons[] = {
    BTN_CH3,
    BTN_BACK,
    BTN_END,
    BTN_TRIM_LEFT,  BTN_TRIM_RIGHT,
    BTN_TRIM_FWD,   BTN_TRIM_BCK,
    BTN_TRIM_CH3_L, BTN_TRIM_CH3_R,
    BTN_DR_L,       BTN_DR_R,
};
#define KEY_BUTTONS_SIZE  (sizeof(key_buttons) / sizeof(u16))




// functions assignable to trims
typedef struct {
    u8 idx;		// will be showed sorted by this
    u8 *name;		// showed identification
    u8 flags;		// bits below
    void (*func)(s16 param, u8 flags);	// function to process key, flags below
    s16 param;		// param given to function
} key_functions_s;
// flags bits
#define KF_NONE		0
#define KF_2STATE	0b00000001
// func flags bits
#define FF_SET		0b00000001
#define FF_ON		0b00000010
#define FF_REVERSE	0b00000100
#define FF_MID		0b00001000
#define FF_SHOW		0b10000000




// set channel value
static void kf_channel(s16 channel, u8 flags) {
    s8 *aval = &menu_channel3_8[channel - 3];

    if (flags & FF_SET) {
	// set value based on state
	if (flags & FF_ON)
	    *aval = (s8)(flags & FF_REVERSE ? -100 : 100);
	else
	    *aval = (s8)(flags & FF_REVERSE ? 100 : -100);
	// check CH3 midle position
	if (flags & FF_MID)  *aval = 0;
    }
    else {
	// switch to opposite value
	if (*aval > 0)  *aval = -100;
	else		*aval = 100;
    }
    if (flags & FF_SHOW) {
	lcd_segment(LS_SYM_CHANNEL, LS_ON);
	lcd_7seg((u8)channel);
	lcd_char_num3(*aval);
    }
}

// reset channel value to 0
static void kf_channel_reset(s16 channel, u8 flags) {
    s8 *aval = &menu_channel3_8[channel - 3];

    *aval = 0;
    if (flags & FF_SHOW) {
	lcd_segment(LS_SYM_CHANNEL, LS_ON);
	lcd_7seg((u8)channel);
	lcd_char_num3(*aval);
    }
}

// change 4WS crab/no-crab
static void kf_4ws(s16 unused, u8 flags) {
    if (flags & FF_SET) {
	if (flags & FF_ON)
	    menu_4WS_crab = (u8)(flags & FF_REVERSE ? 0 : 1);
	else
	    menu_4WS_crab = (u8)(flags & FF_REVERSE ? 1 : 0);
    }
    else
	menu_4WS_crab ^= 1;
    if (flags & FF_SHOW) {
	lcd_7seg(4);
	lcd_chars(menu_4WS_crab ? "CRB" : "NOC");
    }
}




// table of key functions
static const key_functions_s key_functions[] = {
    { 0, "OFF", KF_NONE, 0 },
    { 1, "CH3", KF_2STATE, kf_channel, 3 },
    { 7, "C3R", KF_NONE, kf_channel_reset, 3 },
#if MAX_CHANNELS >= 4
    { 2, "CH4", KF_2STATE, kf_channel, 4 },
    { 8, "C4R", KF_NONE, kf_channel_reset, 4 },
#if MAX_CHANNELS >= 5
    { 3, "CH5", KF_2STATE, kf_channel, 5 },
    { 9, "C5R", KF_NONE, kf_channel_reset, 5 },
#if MAX_CHANNELS >= 6
    { 4, "CH6", KF_2STATE, kf_channel, 6 },
    { 10, "C6R", KF_NONE, kf_channel_reset, 6 },
#if MAX_CHANNELS >= 7
    { 5, "CH7", KF_2STATE, kf_channel, 7 },
    { 11, "C7R", KF_NONE, kf_channel_reset, 7 },
#if MAX_CHANNELS >= 8
    { 6, "CH8", KF_2STATE, kf_channel, 8 },
    { 12, "C8R", KF_NONE, kf_channel_reset, 8 },
#endif
#endif
#endif
#endif
#endif
    { 13, "4WS", KF_2STATE, kf_4ws, 0 },
};
#define KEY_FUNCTIONS_SIZE  (sizeof(key_functions) / sizeof(key_functions_s))


// return name of given line
u8 *menu_key_function_name(u8 n) {
    return key_functions[n].name;
}

// return sort idx of given line
s8 menu_key_function_idx(u8 n) {
    if (n >= KEY_FUNCTIONS_SIZE)  return -1;
    return key_functions[n].idx;
}

// return if it is 2-state function
u8 menu_key_function_2state(u8 n) {
    return (u8)(key_functions[n].flags & KF_2STATE);
}




// change val, temporary show new value (not always)
// end when another key pressed
static u8 menu_popup_key(u8 key_id) {
    u16 delay_time;
    u16 buttons_state_last;
    u16 btnx;
    config_key_map_s *km = &ck.key_map[key_id];
    key_functions_s *kf;
    key_functions_s *kfl;
    u8 flags;
    u8 is_long = 0;

    // do nothing when both short and long set to OFF
    if (!km->function && !km->function_long)  return 0;

    // prepare more variables
    kf = &key_functions[km->function];
    btnx = key_buttons[key_id];

    // check momentary setting
    if (km->function && (kf->flags & KF_2STATE) && km->momentary) {
	flags = FF_SET;
	if (btns(btnx))			flags |= FF_ON;
	if (km->reverse)		flags |= FF_REVERSE;
	if (key_id == 0 && adc_ch3_last > 256 && adc_ch3_last < 768)
					flags |= FF_MID;
	kf->func(kf->param, flags);	// set value to state
	return 0;
    }

    // return when key was not pressed
    if (!btn(btnx))	 return 0;

    kfl = &key_functions[km->function_long];

    // remember buttons state
    buttons_state_last = buttons_state & ~btnx;

    // clear some lcd segments
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_segment(LS_SYM_CHANNEL, LS_OFF);
    lcd_set(L7SEG, LB_EMPTY);

    while (1) {
	if (km->function_long && btnl(btnx)) {
	    // long key press
	    key_beep();
	    kfl->func(kfl->param, FF_SHOW);	// switch value
	    lcd_update();
	    is_long = 1;
	}
	else if (km->function && btn(btnx)) {
	    // short key press
	    key_beep();
	    kf->func(kf->param, FF_SHOW);	// switch value
	    lcd_update();
	}
	else {
	    // nothink to do
	    btnr(btnx);
	    break;
	}
	btnr(btnx);

	// if another button was pressed, leave this screen
	if (buttons)  break;
	if ((buttons_state & ~btnx) != buttons_state_last)  break;

	// sleep 5s, and if no button was changed during, end this screen
	delay_time = POPUP_DELAY * 200;
	while (delay_time && !buttons &&
	       ((buttons_state & ~btnx) == buttons_state_last))
	    delay_time = delay_menu(delay_time);

	if (!buttons)  break;  // timeouted without button press
	if (is_long) {
	    // if long required but short pressed, end
	    if (!btnl(btnx))  break;
	}
	else {
	    // if short required, but long pressed with function_long
	    //   specified, end
	    if (km->function_long && btnl(btnx))  break;
	}
    }

    // set MENU off
    lcd_menu(0);

    return 1;  // popup was showed
}




// check buttons CH3, BACK, END, invoke popup to show value
// return 1 when popup was activated
u8 menu_buttons(void) {
    u8 i;

    // for each key, call function
    for (i = 0; i < KEY_BUTTONS_SIZE; i++) {
	if (i >= NUM_KEYS && ck.key_map[i].is_trim)
	    continue;	// trim is enabled for this key
	if (menu_popup_key(i))  return 1;
    }

    return 0;
}

