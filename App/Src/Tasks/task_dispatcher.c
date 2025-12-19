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
#include <stdlib.h>



// --- Глобальная переменная для хранения последнего распарсенного задания ---
// Это упрощение для нашего теста. В реальной системе здесь мог бы быть пул заданий.
static Job_t g_current_job;

// --- Внешние переменные ---
extern TIM_HandleTypeDef htim3;

// --- Вспомогательные функции для парсинга ---
static bool jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return true;
		}
	return false;
	}

static int json_to_int(const char *json, jsmntok_t *tok) {
	char buf[16] = {0};
	int len = tok->end - tok->start;
	if (len > 0 && len < sizeof(buf)) {
		strncpy(buf, json + tok->start, len);
		return atoi(buf);
		}
	return 0;
	}

static ActionType_t action_from_string(const char* action_str) {
	if (strcmp(action_str, "SET_PUMP_STATE") == 0)
		return ACTION_SET_PUMP_STATE;
	if (strcmp(action_str, "MOVE_ABSOLUTE") == 0)
		return ACTION_ROTATE_MOTOR; // Примерное соответствие
	// ... другие action ...
	return ACTION_NONE;
	}

// --- Основная функция парсинга ---
// Эта функция будет вызываться из task_dispatcher
static bool parse_execute_job_command(const char* json_string, jsmntok_t *tokens, int num_tokens, Job_t* job)

{
	 memset(job, 0, sizeof(Job_t));

	 // Проходим по ключам корневого объекта
	 for (int i = 1; i < num_tokens; i++) {
		 if (jsoneq(json_string, &tokens[i], "request_id")) {
			 int len = tokens[i + 1].end - tokens[i + 1].start;
			 if (len < sizeof(job->request_id)) {
				 strncpy(job->request_id, json_string + tokens[i + 1].start, len);
				 }
			 i++;
               }
		 else if (jsoneq(json_string, &tokens[i], "params")) {
			 jsmntok_t *params_obj = &tokens[i + 1];
			 if (params_obj->type != JSMN_OBJECT)
				 return false;

			 // Ищем 'steps' внутри 'params'
			 for (int j = 1; j <= params_obj->size * 2; j += 2) {
				 jsmntok_t *key = &tokens[i + 1 + j];
				 if (jsoneq(json_string, key, "steps")) {
					 jsmntok_t *steps_array = key + 1;
					 if (steps_array->type != JSMN_ARRAY) return false;
					 job->num_steps = steps_array->size;
					 int current_token = (key + 1) - tokens + 1;
					 // Проходим по каждому шагу в массиве 'steps'
					 for (int k = 0; k < job->num_steps; k++) {
						 jsmntok_t *step_obj = &tokens[current_token];
						 char action_str[32] = {0};
						 jsmntok_t *step_params_obj = NULL;

						 // Ищем 'action' и 'params' внутри шага
						 for(int l = 1; l <= step_obj->size * 2; l+=2) {
							 jsmntok_t *step_key = &tokens[current_token + l];
							 if (jsoneq(json_string, step_key, "action")) {
								 jsmntok_t *val = step_key + 1;
								 strncpy(action_str, json_string + val->start, val->end - val->start);
								 }
							 else if (jsoneq(json_string, step_key, "params")) {
								 step_params_obj = step_key + 1;
								 }
							 }
						 job->steps[k].action = action_from_string(action_str);

						 // Парсим параметры для конкретного action
						 if (job->steps[k].action == ACTION_SET_PUMP_STATE && step_params_obj != NULL) {
							 for(int m = 1; m <= step_params_obj->size * 2; m+=2) {
								 jsmntok_t *p_key = &tokens[(step_params_obj - tokens) + m];
								 if (jsoneq(json_string, p_key, "pump_id")) {
									 job->steps[k].params.pump.pump_id = json_to_int(json_string, p_key + 1);
									 }
								 else if (jsoneq(json_string, p_key, "state")) {
									 job->steps[k].params.pump.state = json_to_int(json_string, p_key + 1);
									 }
								 }
							 }
						 current_token += (step_obj->size * 2) + 1;
						 }
					 return true; // Успешно распарсили
					 }
				 }
			 i += params_obj->size * 2;
			 }
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
		memset(rx_usb_buffer, 0, sizeof(rx_usb_buffer));
		if (xQueueReceive(usb_rx_queue_handle, (void *)rx_usb_buffer, portMAX_DELAY) == pdPASS)
			{
			__HAL_TIM_SET_COUNTER(&htim3, 0);
			uint32_t start_time = __HAL_TIM_GET_COUNTER(&htim3);
			jsmn_init(&parser);
			int num_tokens = jsmn_parse(&parser, rx_usb_buffer, strlen(rx_usb_buffer), tokens, MAX_JSON_TOKENS);
			if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
				Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"Invalid JSON\"}");
				continue;
				}
			char command_name[CMD_BUFFER_LEN] = {0};
			for (int i = 1; i < num_tokens; i++) {
				if (jsoneq(rx_usb_buffer, &tokens[i], "command")) {
					int len = tokens[i + 1].end - tokens[i + 1].start;
					if (len < sizeof(command_name)) {
						strncpy(command_name, rx_usb_buffer + tokens[i + 1].start, len);
						}
					break;
					}
				}
			if (strcmp(command_name, "EXECUTE_JOB") == 0) {
				if (parse_execute_job_command(rx_usb_buffer, tokens, num_tokens, &g_current_job)) {
					// --- ИСПРАВЛЕННЫЙ ВЫЗОВ С 3 АРГУМЕНТАМИ ---
					CommandHandler_Execute("EXECUTE_JOB", (void*)&g_current_job, start_time);
					}
				else {
					Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"Failed to parse EXECUTE_JOB params\"}");
					}
				}
			else {
				// Пока что игнорируем другие команды для простоты теста
				Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"Only EXECUTE_JOB is supported in this test\"}");
				}
			}
		}
}














