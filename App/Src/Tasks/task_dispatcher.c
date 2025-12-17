/*
 * task_dispatcher.c
 *
 *  Created on: Nov 26, 2025
 *      Author: andrey
 */

#include <main.h>
#include"task_dispatcher.h"
#include "cmsis_os.h"
#include "shared_resources.h"
#include "app_config.h"
#include "Dispatcher/command_parser.h"
#include "Dispatcher/dispatcher_io.h"
#include "Dispatcher/job_manager.h"
#include "jsmn.h"        // Для парсинга JSON
#include <string.h>      // Для strcmp, strncmp
#include "job_format.h"
#include <stdio.h>       // Для snprintf
#include "recipe_store.h"
#include "command_handler.h"


// --- Внешние переменные ---
extern TIM_HandleTypeDef htim3; // Таймер для измерения времени парсинга

// --- Вспомогательная функция для сравнения JSON-токена со строкой ---
static bool jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return true;
		}
	return false;
}

void app_start_task_dispatcher(void *argument)
{
	char rx_usb_buffer[APP_USB_CMD_MAX_LEN];
	jsmn_parser parser;
	jsmntok_t tokens[MAX_JSON_TOKENS];

	for(;;)
		{
		// ОЧИСТИТЬ БУФЕР <<<
		memset(rx_usb_buffer, 0, sizeof(rx_usb_buffer));

		if (xQueueReceive(usb_rx_queue_handle, (void *)rx_usb_buffer, portMAX_DELAY) == pdPASS)
			{
			// --- Начало измерения времени ---
			__HAL_TIM_SET_COUNTER(&htim3, 0);
			uint32_t start_time = __HAL_TIM_GET_COUNTER(&htim3);

			// 1. Токенизация
			jsmn_init(&parser);
			int num_tokens = jsmn_parse(&parser, rx_usb_buffer, strlen(rx_usb_buffer), tokens, MAX_JSON_TOKENS);

		    if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
		    	Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"Invalid JSON: root is not an object\"}");
		    	continue;
		    	}

		    // 2. Извлекаем основные ключи
		    char command_name[CMD_BUFFER_LEN] = {0};
		    char request_id[REQ_ID_BUFFER_LEN] = {0};

		    jsmntok_t *params_token = NULL;

		    // Проходим по ключам корневого объекта
		    for (int i = 1; i < num_tokens; i++) {
		    	if (jsoneq(rx_usb_buffer, &tokens[i], "command")) {
		    		int len = tokens[i + 1].end - tokens[i + 1].start;
		    		if (len < sizeof(command_name)) {
		    			strncpy(command_name, rx_usb_buffer + tokens[i + 1].start, len);
		    			command_name[len] = '\0';
		    			}
		    		i++;
		    		}
		    	else if (jsoneq(rx_usb_buffer, &tokens[i], "request_id"))
		    	{
		    		int len = tokens[i + 1].end - tokens[i + 1].start;
		    		if (len < sizeof(request_id)) {
		    			strncpy(request_id, rx_usb_buffer + tokens[i + 1].start, len);
		    			request_id[len] = '\0';
		    			}
		    		i++;
		    		}
		    	else if (jsoneq(rx_usb_buffer, &tokens[i], "params"))
		    	{
		    		params_token = &tokens[i + 1];
		    		// Пропускаем все дочерние токены объекта params, чтобы не обрабатывать их здесь
		    		int params_end = params_token->end;
		    		while(i < num_tokens - 1 && tokens[i+1].start < params_end)
		    		{
		    			i++;
		    			}
		    		}
		    	}
		    // 3. Передаем управление в CommandHandler
		    if (strlen(command_name) > 0) {

		    	// --- КОНЕЦ ИЗМЕРЕНИЯ ВРЕМЕНИ ---
		        // Измеряем общее время от начала до момента перед вызовом обработчика
		    	uint32_t total_processing_time_us = __HAL_TIM_GET_COUNTER(&htim3) - start_time;
		    	CommandHandler_Execute(command_name, request_id, params_token, rx_usb_buffer, num_tokens, total_processing_time_us);

		    	}
		    else
		    {
		    	Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"'command' field not found\"}");
		    	}
		    }
		}
}








