/*
 * command_handler.c
 *
 *  Created on: Dec 16, 2025
 *      Author: andrey
 */


#include "Dispatcher/command_handler.h"
#include "Dispatcher/dispatcher_io.h"
#include "job_format.h"       // Для структуры Job_t
#include "can_message.h"      // Для CanMessage_t
#include "recipe_store.h"     // Для ActionType_t
#include "shared_resources.h" // Для can_tx_queue_handle
#include "main.h"             // Для HAL_... и FDCAN_...
#include <string.h>
#include <stdio.h>


// --- Внешние переменные ---
extern TIM_HandleTypeDef htim3;


// --- Определения обработчиков и таблицы ---
typedef void (*CommandHandler_t)(void *data, uint32_t start_time); // Обновленный тип
static void handle_ping(void *data, uint32_t start_time);
static void handle_execute_job(void *data, uint32_t start_time);

typedef struct {
	const char* command_name;
	CommandHandler_t handler_func;
	} CommandDispatch_t;

	static const CommandDispatch_t dispatch_table[] = {
			{ "PING",        (CommandHandler_t)handle_ping },
			{ "EXECUTE_JOB", (CommandHandler_t)handle_execute_job }
			};

	// --- CommandHandler_Execute (обновленный) ---
	void CommandHandler_Execute(const char* command_name, void* data, uint32_t start_time)
	{
		for (size_t i = 0; i < sizeof(dispatch_table) / sizeof(dispatch_table[0]); i++) {
			if (strcmp(command_name, dispatch_table[i].command_name) == 0) {
				dispatch_table[i].handler_func(data, start_time);
				return;
				}
			}
		// Обработка неизвестной команды (если нужно)
		}
	// --- Обработчики команд (обновленные) ---
	static void handle_ping(void *data, uint32_t start_time)
	{
		char response[128];
		uint32_t final_time = __HAL_TIM_GET_COUNTER(&htim3) - start_time;
		snprintf(response, sizeof(response),
				"{\"status\":\"OK\", \"message\":\"PONG\", \"total_time_us\":%lu}",
				(unsigned long)final_time);
		Dispatcher_SendUsbResponse(response);
		}
	static void handle_execute_job(void *data, uint32_t start_time)
	{
		if (data == NULL) {
			Dispatcher_SendUsbResponse("{\"status\":\"ERROR\", \"message\":\"Job data is NULL\"}");
			return;
			}
		Job_t* job = (Job_t*)data;
		bool can_sent = false;

		// Проходим по шагам задания
		for (int i = 0; i < job->num_steps; i++) {
			JobStep_t* step = &job->steps[i];
			// Используем switch для обработки разных действий
			switch (step->action) {
			   case ACTION_SET_PUMP_STATE:
				   {
					   // Собираем CAN-сообщение для насоса
					   CanMessage_t can_msg;
					   // 1. Заголовок

					   can_msg.Header.IdType = FDCAN_STANDARD_ID;
					   // ID исполнителя насосов = 1 (согласно нашей договоренности)
					   // ID насоса берем из параметров шага
					   can_msg.Header.Identifier = 0x100 | (1 << 4) | (step->params.pump.pump_id & 0x07);
					   can_msg.Header.TxFrameType = FDCAN_DATA_FRAME;
					   can_msg.Header.DataLength = FDCAN_DLC_BYTES_2; // Команда (1 байт) + Состояние (1 байт)
					   // 2. Данные
					   // ВАЖНО: Нам нужен реальный ID команды из протокола CAN.
					   // Пока что используем заглушку 0x10.
					   can_msg.Data[0] = ACTION_SET_PUMP_STATE; // Используем ID из ActionType_t
					   can_msg.Data[1] = step->params.pump.state; // 1 (ON) или 0 (OFF)
					   // 3. Отправляем в CAN очередь
					   if (xQueueSend(can_tx_queue_handle, &can_msg, pdMS_TO_TICKS(100)) == pdPASS) {
						   can_sent = true;
						   // >>> ПЯТЬ МИГОВ: CAN-фрейм получен задачей для отправки <<<
						      for(int i=0; i<10; i++) { HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); osDelay(50); }

						      }
					   break;
					   }
				   // Здесь будут другие case для ACTION_MOVE_ABSOLUTE и т.д.
				   default:
					   // Неизвестное действие
					   break;
					   }
			}
		// --- Измерение времени и отправка финального ответа ---
		uint32_t final_time = __HAL_TIM_GET_COUNTER(&htim3) - start_time;
		char response[128];
		snprintf(response, sizeof(response),
				"{\"status\":\"OK\", \"can_sent\":%s, \"total_time_us\":%lu}",
				can_sent ? "true" : "false", (unsigned long)final_time);
		Dispatcher_SendUsbResponse(response);
		}





































