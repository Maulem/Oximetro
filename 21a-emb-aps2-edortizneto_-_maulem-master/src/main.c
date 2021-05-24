/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
#include "aps2/aps2.h"
#include "aps2/ecg.h"

/************************************************************************/
/* Defines                                                              */
/************************************************************************/

#define TASK_LCD_STACK_SIZE         (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_PRIORITY           (tskIDLE_PRIORITY)

#define TASK_APS2_STACK_SIZE        (1024*6/sizeof(portSTACK_TYPE))
#define TASK_APS2_PRIORITY          (tskIDLE_PRIORITY)

#define TASK_MAIN_STACK_SIZE        (1024*6/sizeof(portSTACK_TYPE))
#define TASK_MAIN_PRIORITY          (tskIDLE_PRIORITY)

// Canal do pino PC31
#define AFEC_POTY					AFEC1
#define AFEC_POT_IDY				ID_AFEC1
#define AFEC_POT_CHANNELY			6

/************************************************************************/
/* Declares                                                             */
/************************************************************************/

LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg60);
LV_FONT_DECLARE(dseg50);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg30);
LV_IMG_DECLARE(doctian);

/************************************************************************/
/* Structs                                                              */
/************************************************************************/

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

typedef struct {
	uint value;
} adcData;

typedef struct {
	uint ecg;
	uint bpm;
} ecgInfo;

/************************************************************************/
/* Static                                                               */
/************************************************************************/

/*A static or global variable to store the buffers*/
static lv_disp_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];

static  lv_obj_t * labelClock;
static  lv_obj_t * labelOx;
static  lv_obj_t * labelOxSaved;
static  lv_obj_t * labelBpm;
static  lv_obj_t * labelBpmSaved;
static  lv_obj_t * labelOxo2;
static  lv_obj_t * labelBpmText;
static  lv_obj_t * labelBpmHeart;
static  lv_obj_t * Changegraph;
static  lv_obj_t * labelPlus;
static  lv_obj_t * labelMinus;
static  lv_obj_t * labelUp;
static  lv_obj_t * labelDown;
/************************************************************************/
/* Semaphore / Queue                                                    */
/************************************************************************/
SemaphoreHandle_t xSemaphoreClock;
SemaphoreHandle_t xSemaphoreSave;
SemaphoreHandle_t xSemaphoreAlarm;
SemaphoreHandle_t xSemaphoreSaveBpm;
SemaphoreHandle_t xSemaphoreChangeGraph;
SemaphoreHandle_t xSemaphoreUp;
SemaphoreHandle_t xSemaphoreDown;
SemaphoreHandle_t xSemaphorePlus;
SemaphoreHandle_t xSemaphoreMinus;

QueueHandle_t xQueueECG;
QueueHandle_t xQueueEcgInfo;

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile int count = 0;
volatile Bool f_rtt_alarme = true;
volatile uint32_t g_ul_valueECG = 0;

// Grafico
#define CHAR_DATA_LEN 250
int ser1_data[CHAR_DATA_LEN];
lv_obj_t * chart;
lv_chart_series_t * ser1;
int ser21_data[CHAR_DATA_LEN];
lv_obj_t * chart2;
lv_chart_series_t * ser21;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
  printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
  for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {  configASSERT( ( volatile void * ) NULL ); }

/************************************************************************/
/* Prototypes                                                           */
/************************************************************************/

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, 
	afec_callback_t callback);
void lv_screen_chart(void);
static float get_time_rtt();
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);


/************************************************************************/
/* ASP2 - NAO MEXER!                                                    */
/************************************************************************/

QueueHandle_t xQueueOx;
TimerHandle_t xTimer;
volatile int g_ecgCnt = 0;
volatile int g_ecgDelayCnt = 0;
volatile int g_ecgDelayValue = 0;
const int g_ecgSize =  sizeof(ecg)/sizeof(ecg[0]);

/************************************************************************/
/* callback/Handlers                                                    */
/************************************************************************/

void TC3_Handler(void) 
{
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC1, 0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/* Seleciona canal e inicializa conversão */
	afec_channel_enable(AFEC_POTY, AFEC_POT_CHANNELY);
	afec_start_software_conversion(AFEC_POTY);
}

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		//f_rtt_alarme = false;
		//pin_toggle(LED_2_PIO, LED_2_IDX_MASK);    // BLINK Led
		count++;
	}

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		//count++;
		//f_rtt_alarme = true;                  // flag RTT alarme
	}
}

static void AFEC_pot_CallbackY(void){
	g_ul_valueECG = afec_channel_get_value(AFEC_POTY, AFEC_POT_CHANNELY);
	//BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(conversion_SemaphoreY, &xHigherPriorityTaskWoken);
	
	adcData ecgRead;
	ecgRead.value = g_ul_valueECG;
	xQueueSendFromISR(xQueueECG, &ecgRead, 0);
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/* seccond tick	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreClock, &xHigherPriorityTaskWoken);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		//flag_rtc = 1;
	}
	
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

static void ox_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreSave, &xHigherPriorityTaskWoken);
	}
}

static void bpm_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreSaveBpm, &xHigherPriorityTaskWoken);
	}
}

static void change_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreChangeGraph, &xHigherPriorityTaskWoken);
	}
}

static void alarm_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_VALUE_CHANGED) {
		printf("Toggled\n");
	}
}

static void down_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreDown, &xHigherPriorityTaskWoken);
	}
}

static void plus_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphorePlus, &xHigherPriorityTaskWoken);
	}
}

static void minus_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreMinus, &xHigherPriorityTaskWoken);
	}
}

static void up_handler(lv_obj_t * obj, lv_event_t event) {
	if(event == LV_EVENT_CLICKED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreUp, &xHigherPriorityTaskWoken);
	}
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

void lv_logo(void) {
	//-------------------
	// Logotipo
	//-------------------
		
	lv_obj_t * img1 = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(img1, &doctian);
	lv_obj_align(img1, NULL, LV_ALIGN_IN_TOP_MID, 0, -10);
}

void lv_main(void) {
	lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_logo();
}

// Desenha gráfico no LCD
void lv_screen_chart(void) {
	chart = lv_chart_create(lv_scr_act(), NULL);
	lv_obj_set_size(chart, 150, 95);
	lv_obj_align(chart, NULL, LV_ALIGN_CENTER, 80, -20);
	lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
	lv_chart_set_range(chart, 0, 4095);
	lv_chart_set_point_count(chart, CHAR_DATA_LEN);
	lv_chart_set_div_line_count(chart, 0, 0);
	lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

	ser1 = lv_chart_add_series(chart, LV_COLOR_BLUE);
	lv_chart_set_ext_array(chart, ser1, ser1_data, CHAR_DATA_LEN);
	lv_obj_set_style_local_line_width(chart, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 1);
	lv_obj_set_style_local_size(chart, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_DPI/150);
}

void lv_screen_chart_extra(void) {
	chart2 = lv_chart_create(lv_scr_act(), NULL);
	lv_obj_set_size(chart2, 320, 194);
	lv_obj_align(chart2, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
	lv_chart_set_type(chart2, LV_CHART_TYPE_LINE);
	lv_chart_set_range(chart2, 0, 4095);
	lv_chart_set_point_count(chart2, CHAR_DATA_LEN);
	lv_chart_set_div_line_count(chart2, 0, 0);
	lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_SHIFT);

	ser21 = lv_chart_add_series(chart2, LV_COLOR_BLUE);
	lv_chart_set_ext_array(chart2, ser21, ser21_data, CHAR_DATA_LEN);
	lv_obj_set_style_local_line_width(chart2, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 1);
	lv_obj_set_style_local_size(chart2, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_DPI/150);
}


/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

void task_RTC(void *pvParameters) {
	
	xSemaphoreClock = xSemaphoreCreateBinary();
	xSemaphoreUp = xSemaphoreCreateBinary();
	xSemaphoreDown = xSemaphoreCreateBinary();
	
	// Configura RTC 
	calendar rtc_initial = {2021, 4, 19, 12, 13, 39 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	labelClock = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelClock, NULL, LV_ALIGN_IN_TOP_RIGHT, -90, 5);
	lv_obj_set_style_local_text_font(labelClock, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &dseg30);
	lv_obj_set_style_local_text_color(labelClock, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	
	char * clock_char_buffer;
	int clock_int_buffer = 0;
	while(1){
		if( xSemaphoreTake(xSemaphoreClock, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			rtc_get_time(RTC, &rtc_initial.hour, &rtc_initial.minute, &rtc_initial.second);
			lv_label_set_text_fmt(labelClock, "%02u : %02u", rtc_initial.hour, rtc_initial.minute);
		}
		if( xSemaphoreTake(xSemaphoreUp, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			rtc_initial.minute++;
			//printf("%d", rtc_initial.minute);
			rtc_set_time(RTC, rtc_initial.hour, rtc_initial.minute, rtc_initial.second);
		}
		if( xSemaphoreTake(xSemaphoreDown, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			rtc_initial.minute--;
			rtc_set_time(RTC, rtc_initial.hour, rtc_initial.minute, rtc_initial.second);
		}
	}
}

static void task_lcd(void *pvParameters) {
  
  lv_screen_chart_extra();
  lv_screen_chart();
  lv_main();
  
  for (;;)  {
    lv_tick_inc(50);
    lv_task_handler();
    vTaskDelay(50);
  }
}

static void task_ox(void *pvParameters) {
	
	xSemaphoreSave = xSemaphoreCreateBinary();
	xSemaphoreSaveBpm = xSemaphoreCreateBinary();
	xSemaphoreChangeGraph = xSemaphoreCreateBinary();
	xSemaphorePlus = xSemaphoreCreateBinary();
	xSemaphoreMinus = xSemaphoreCreateBinary();
	
	
	// Oximeter gauge
	static lv_color_t needle_colors[1];
	needle_colors[0] = LV_COLOR_LIME;
	
	lv_obj_t * gaugeOx = lv_gauge_create(lv_scr_act(), NULL);
	lv_gauge_set_needle_count(gaugeOx, 1, needle_colors);
	lv_obj_set_size(gaugeOx, 160, 160);
	lv_obj_align(gaugeOx, NULL, LV_ALIGN_CENTER, -80, 25);
	lv_gauge_set_range(gaugeOx, 85, 100);
	lv_gauge_set_critical_value(gaugeOx, 90);
	
	// Save OX button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnOx = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnOx, ox_handler);
	lv_obj_set_width(btnOx, 60);  lv_obj_set_height(btnOx, 60);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnOx, NULL, LV_ALIGN_CENTER, -80, 25);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE );
	lv_obj_set_style_local_border_color(btnOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	
	// Oximeter value
	labelOx = lv_label_create(btnOx, NULL);
	//lv_obj_align(labelOx, NULL, LV_ALIGN_CENTER, -80, 50);
	lv_obj_set_style_local_text_font(labelOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &dseg30);
	
	// Oximeter text
	labelOxo2 = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelOxo2, NULL, LV_ALIGN_CENTER, -75, 66);
	lv_obj_set_style_local_text_color(labelOxo2, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_label_set_text_fmt(labelOxo2, "O2");
	
	// Oximeter Saved value
	labelOxSaved = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelOxSaved, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 5);
	lv_obj_set_style_local_text_font(labelOxSaved, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &dseg30);
	lv_label_set_text_fmt(labelOxSaved, "%s", "--");
	
	// Change graph button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnGraf = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnGraf, change_handler);
	lv_obj_set_width(btnGraf, 30);  lv_obj_set_height(btnGraf, 30);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnGraf, NULL, LV_ALIGN_CENTER, 20, -50);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnGraf, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_color(btnGraf, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnGraf, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	Changegraph = lv_label_create(btnGraf, NULL);
	lv_label_set_recolor(Changegraph, true);
	lv_label_set_text(Changegraph, "#ffffff  " LV_SYMBOL_SHUFFLE " #");
	
	// Save Bpm button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnBpm = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnBpm, bpm_handler);
	lv_obj_set_width(btnBpm, 70);  lv_obj_set_height(btnBpm, 70);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnBpm, NULL, LV_ALIGN_CENTER, 120, 104);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnBpm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE );
	lv_obj_set_style_local_border_color(btnBpm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnBpm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	
	// Bpm value
	labelBpm = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelBpm, NULL, LV_ALIGN_CENTER, 100, 86);
	lv_obj_set_style_local_text_font(labelBpm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &dseg40);
	lv_label_set_text_fmt(labelBpm, "%s", "--");
	lv_obj_set_style_local_text_color(labelBpm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	
	// Bpm value text
	labelBpmText = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelBpmText, NULL, LV_ALIGN_CENTER, 60, 112);
	lv_label_set_text_fmt(labelBpmText, "BPM:");
	lv_obj_set_style_local_text_color(labelBpmText, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	
	// Bpm Saved value
	labelBpmSaved = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(labelBpmSaved, NULL, LV_ALIGN_CENTER, 120, 40);
	lv_obj_set_style_local_text_font(labelBpmSaved, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &dseg30);
	lv_label_set_text_fmt(labelBpmSaved, "%s", "--");
	lv_obj_set_style_local_text_color(labelBpmSaved, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
	
	// Limit plus button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnPlus = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnPlus, plus_handler);
	lv_obj_set_width(btnPlus, 30);  lv_obj_set_height(btnPlus, 30);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnPlus, NULL, LV_ALIGN_CENTER, -20, 100);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnPlus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_color(btnPlus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnPlus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	labelPlus = lv_label_create(btnPlus, NULL);
	lv_label_set_recolor(labelPlus, true);
	lv_label_set_text(labelPlus, "#ffffff  " LV_SYMBOL_PLUS " #");
	
	// Limit less button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnMinus = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnMinus, minus_handler);
	lv_obj_set_width(btnMinus, 30);  lv_obj_set_height(btnMinus, 30);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnMinus, NULL, LV_ALIGN_CENTER, -140, 100);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnMinus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_color(btnMinus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnMinus, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	labelMinus = lv_label_create(btnMinus, NULL);
	lv_label_set_recolor(labelMinus, true);
	lv_label_set_text(labelMinus, "#ffffff  " LV_SYMBOL_MINUS " #");
	
	// Clock up button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnUp = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnUp, up_handler);
	lv_obj_set_width(btnUp, 30);  lv_obj_set_height(btnUp, 30);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnUp, NULL, LV_ALIGN_CENTER, 30, 50);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnUp, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_color(btnUp, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnUp, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	labelUp = lv_label_create(btnUp, NULL);
	lv_label_set_recolor(labelUp, true);
	lv_label_set_text(labelUp, "#ffffff  " LV_SYMBOL_UP " #");
	
	// Clock down button
	// cria botao de tamanho 60x60 redondo
	lv_obj_t * btnDown = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnDown, down_handler);
	lv_obj_set_width(btnDown, 30);  lv_obj_set_height(btnDown, 30);

	// alinha no canto esquerdo e desloca um pouco para cima e para direita
	lv_obj_align(btnDown, NULL, LV_ALIGN_CENTER, 30, 90);

	// altera a cor de fundo, borda do botão criado para PRETO
	lv_obj_set_style_local_bg_color(btnDown, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_color(btnDown, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK );
	lv_obj_set_style_local_border_width(btnDown, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	labelDown = lv_label_create(btnDown, NULL);
	lv_label_set_recolor(labelDown, true);
	lv_label_set_text(labelDown, "#ffffff  " LV_SYMBOL_DOWN " #");
	
	char ox;
	int last_ox;
	int limit = 90;
	
	adcData ecgRead;
	ecgInfo receive;
	
	int graf = 1;
	
	for (;;)  {
		if ( xQueueReceive( xQueueEcgInfo , &(receive), 0 )) {
			
			//printf("ecg: %d \n", receive.ecg);
			//printf("bpm:  %d\n", receive.bpm);
			switch(graf) {
				case 1:
					lv_chart_set_next(chart, ser1, receive.ecg);
					break;
				case 2:
					lv_chart_set_next(chart, ser1, ox);
					break;
				default:
					printf("Graph error!");
					break;
			}
			
			
			lv_chart_refresh(chart);
			
			lv_label_set_text_fmt(labelBpm, "%d", receive.bpm);
		}
		
		if ( xQueueReceive( xQueueOx, &ox, 0 )) {
			//printf("ox: %d \n", ox);
			if (ox < limit) {
				needle_colors[0] = LV_COLOR_RED;
				lv_gauge_set_value(gaugeOx, 0, ox);
				lv_obj_set_style_local_text_color(labelOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
				lv_label_set_text_fmt(labelOx, "%d", ox);
				if (last_ox >= limit) {
					BaseType_t xHigherPriorityTaskWoken = pdFALSE;
					xSemaphoreGiveFromISR(xSemaphoreAlarm, &xHigherPriorityTaskWoken);
				}
			}
			else {
				needle_colors[0] = LV_COLOR_LIME;
				lv_gauge_set_value(gaugeOx, 0, ox);
				lv_obj_set_style_local_text_color(labelOx, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
				lv_label_set_text_fmt(labelOx, "%d", ox);
			}
			last_ox = ox;
		}
		if( xSemaphoreTake(xSemaphoreSave, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			if (ox < limit) {
				lv_obj_set_style_local_text_color(labelOxSaved, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
				lv_label_set_text_fmt(labelOxSaved, "%d", ox);
			}
			else {
				lv_obj_set_style_local_text_color(labelOxSaved, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
				lv_label_set_text_fmt(labelOxSaved, "%d", ox);
			}
		}
		
		if( xSemaphoreTake(xSemaphoreSaveBpm, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			lv_label_set_text_fmt(labelBpmSaved, "%d", receive.bpm);
		}
		
		
		if( xSemaphoreTake(xSemaphoreChangeGraph, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			if (graf == 1) {
				graf = 2;
				lv_chart_set_range(chart, 80, 100);
			}
			else {
				graf = 1;
				lv_chart_set_range(chart, 0, 4095);
			}
		}
		
		if( xSemaphoreTake(xSemaphorePlus, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			if (limit < 95) {
				limit += 5;
			}
		}
		if( xSemaphoreTake(xSemaphoreMinus, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
			if (limit > 85) {
				limit -= 5;
			}
		}
         
		vTaskDelay(25);
		}
}

static void task_process(void *pvParameters) {
	
	/// Leitura AFEC 1 canal 6 ///
	/* inicializa e configura adc */
	config_AFEC_pot(AFEC_POTY, AFEC_POT_IDY, AFEC_POT_CHANNELY, AFEC_pot_CallbackY);
	
	/* Seleciona canal e inicializa conversão */
	afec_channel_enable(AFEC_POTY, AFEC_POT_CHANNELY);
	afec_start_software_conversion(AFEC_POTY);
	
	Bool pico = false;
	Bool down = false;
	int now;
	double dT;
	
	adcData ecgRead;
	ecgInfo send;
	uint32_t bpm = 99;
	
	for (;;)  {
		
		if ( xQueueReceive( xQueueECG, &(ecgRead), 0 )) {
			if (ecgRead.value > 3280) {
				if (pico == false) {
					now = count;
					pico = true;
				}
				if (down && pico) {
					pico = false;
					down = false;
					now = count - now;
					dT = 60000/now;
					bpm = (int)dT;
				}
			}
			else {
				if (pico == true && down == false) {
					down = true;
				}
			}
			send.ecg = ecgRead.value;
			send.bpm = bpm;
			xQueueSend(xQueueEcgInfo, &send, 0);
		}
		
		if (f_rtt_alarme) {
			/*
			* IRQ 1000Hz
			*/
			uint16_t pllPreScale = (int) (((float) 32768) / 1000.0);
			uint32_t irqRTTvalue = 1;
			// reinicia RTT para gerar um novo IRQ
			RTT_init(pllPreScale, irqRTTvalue);
			f_rtt_alarme = false;
		}
	}
}

static void task_alarm( void *pvParameters) {
	xSemaphoreAlarm = xSemaphoreCreateBinary();
	
	// Alarm
	
	lv_obj_t * btnAlarm = lv_btn_create(lv_scr_act(), NULL);
	lv_obj_set_event_cb(btnAlarm, alarm_handler);
	lv_obj_set_width(btnAlarm, 50);  lv_obj_set_height(btnAlarm, 30);
	lv_obj_align(btnAlarm, NULL, LV_ALIGN_IN_TOP_LEFT, 60, 3);
	lv_btn_set_checkable(btnAlarm, true);
	lv_btn_toggle(btnAlarm);
	
	// altera a cor de fundo, borda do botão criado para Branco
	lv_obj_set_style_local_bg_color(btnAlarm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED );
	lv_obj_set_style_local_border_color(btnAlarm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED );
	lv_obj_set_style_local_border_width(btnAlarm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	
	lv_obj_t * labelAlarm = lv_label_create(btnAlarm, NULL);
	lv_obj_set_style_local_text_color(labelAlarm, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_label_set_text_fmt(labelAlarm," ALARM ");
	
	for (;;) {
		if( xSemaphoreTake(xSemaphoreAlarm, ( TickType_t ) 500 / portTICK_PERIOD_MS) == pdTRUE ) {
			if (lv_btn_get_state(btnAlarm) != LV_BTN_STATE_DISABLED) {
				lv_btn_toggle(btnAlarm);
			}
		}
		vTaskDelay(25);
	}
}
/************************************************************************/
/* Configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
  /**LCD pin configure on SPI*/
  pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
  pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
  pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
  pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
  pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
  pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
  
}

static void configure_console(void) {
  const usart_serial_options_t uart_serial_options = {
    .baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
    .charlength = USART_SERIAL_CHAR_LENGTH,
    .paritytype = USART_SERIAL_PARITY,
    .stopbits = USART_SERIAL_STOP_BIT,
  };

  /* Configure console UART. */
  stdio_serial_init(CONSOLE_UART, &uart_serial_options);

  /* Specify that stdout should not be buffered. */
  setbuf(stdout, NULL);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4); // Prioridade 9
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, afec_callback_t callback){
  /*************************************
  * Ativa e configura AFEC
  *************************************/
  /* Ativa AFEC - 0 */
  afec_enable(afec);

  /* struct de configuracao do AFEC */
  struct afec_config afec_cfg;

  /* Carrega parametros padrao */
  afec_get_config_defaults(&afec_cfg);

  /* Configura AFEC */
  afec_init(afec, &afec_cfg);

  /* Configura trigger por software */
  afec_set_trigger(afec, AFEC_TRIG_SW);

  /*** Configuracao específica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

  /*
  * Calibracao:
  * Because the internal ADC offset is 0x200, it should cancel it and shift
  down to 0.
  */
  afec_channel_set_analog_offset(afec, afec_channel, 0x200);

  /***  Configura sensor de temperatura ***/
  struct afec_temp_sensor_config afec_temp_sensor_cfg;

  afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
  afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);
  
  /* configura IRQ */
  afec_set_callback(afec, afec_channel,	callback, 1);
  NVIC_SetPriority(afec_id, 4);
  NVIC_EnableIRQ(afec_id);
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
	return ul_previous_time;
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	
	/* SUBLINHANDO ESSE WHILE PARA SINCRONIA */
	//while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN | RTT_MR_RTTINCIEN);
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	/* O TimerCounter é meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrupçcão no TC canal 0 */
	/* Interrupção no C */
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	/* Inicializa o canal 0 do TC */
	tc_start(TC, TC_CHANNEL);
}

/************************************************************************/
/* Port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
  ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
  ili9341_copy_pixels_to_screen(color_p,  (area->x2 - area->x1) * (area->y2 - area->y1));
  
  /* IMPORTANT!!!
  * Inform the graphics library that you are ready with the flushing*/
  lv_disp_flush_ready(disp_drv);
}

bool my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
  int px, py, pressed;
  
  if (readPoint(&px, &py)) {
    data->state = LV_INDEV_STATE_PR;
  }
  else {
    data->state = LV_INDEV_STATE_REL;
  }
  
  data->point.x = px;
  data->point.y = py;
  return false; /*No buffering now so no more data read*/
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
  /* board and sys init */
  board_init();
  sysclk_init();
  configure_console();

  /* LCd int */
  configure_lcd();
  ili9341_init();
  configure_touch();
  ili9341_backlight_on();
  
  /*LittlevGL init*/
  lv_init();
  lv_disp_drv_t disp_drv;                 /*A variable to hold the drivers. Can be local variable*/
  lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
  lv_disp_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);  /*Initialize `disp_buf` with the buffer(s) */
  disp_drv.buffer = &disp_buf;            /*Set an initialized buffer*/
  disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
  lv_disp_t * disp;
  disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
  
  /* Init input on LVGL */
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_input_read;
  /*Register the driver in LVGL and save the created input device object*/
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
  
  xQueueOx = xQueueCreate(32, sizeof(char));
  
  /** Configura timer TC1, canal 3 */
  TC_init(TC1, ID_TC3, 0, 250);// 250HZ
  
  xQueueECG = xQueueCreate(250, sizeof(adcData));
  
  xQueueEcgInfo = xQueueCreate(32, sizeof(ecgInfo));

  if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create lcd task\r\n");
  }
  
  if (xTaskCreate(task_aps2, "APS2", TASK_APS2_STACK_SIZE, NULL, TASK_APS2_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create APS task\r\n");
  }
  
  if (xTaskCreate(task_ox, "ox", TASK_MAIN_STACK_SIZE, NULL, TASK_MAIN_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create ox task\r\n");
  }
  
  if (xTaskCreate(task_RTC, "RTC", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create RTC task\r\n");
  }
  
  if (xTaskCreate(task_alarm, "alarm", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create alarm task\r\n");
  }
  
  if (xTaskCreate(task_process, "process", TASK_MAIN_STACK_SIZE, NULL, TASK_MAIN_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create Process task\r\n");
  }
  
  /* Start the scheduler. */
  vTaskStartScheduler();

  /* RTOS n?o deve chegar aqui !! */
  while(1){ }
}
