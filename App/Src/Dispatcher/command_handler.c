/*
 * command_handler.c
 *
 *  Created on: Dec 16, 2025
 *      Author: andrey
 */


#include "Dispatcher/command_handler.h"
#include "Dispatcher/dispatcher_io.h" // Для отправки ответов по USB
#include <string.h>
#include <stdio.h>

// --- Структура для таблицы диспетчеризации ---
typedef struct {
	const char* command_name;
	CommandHandler_t handler_func;
	} CommandDispatch_t;


// --- 1. Объявляем наши будущие функции-обработчики ---
// Каждый для своей команды.
static void handle_ping(const char* request_id, jsmntok_t *params_token, const char* json_string,
		int num_tokens, uint32_t parsing_time_us);
static void handle_execute_job(const char* request_id, jsmntok_t *params_token, const char* json_string,
		int num_tokens, uint32_t parsing_time_us);


// --- 2. Создаем саму таблицу-справочник ---
static const CommandDispatch_t dispatch_table[] = {
		{ "PING",        &handle_ping },
		{ "EXECUTE_JOB", &handle_execute_job }
		// Сюда мы будем добавлять новые команды
		};

// --- 3. Реализуем главную функцию-диспетчер ---
void CommandHandler_Execute(const char* command_name, const char* request_id, jsmntok_t *params_token, const char* json_string,
		int num_tokens, uint32_t parsing_time_us)
{
	// Ищем команду в нашей таблице
	for (size_t i = 0; i < sizeof(dispatch_table) / sizeof(dispatch_table[0]); i++) {
		if (strcmp(command_name, dispatch_table[i].command_name) == 0) {
			// Команда найдена! Вызываем соответствующий обработчик по указателю.
			dispatch_table[i].handler_func(request_id, params_token, json_string, num_tokens, parsing_time_us);
			return; // Выходим из функции, так как команда обработана
			}
}

// Если мы дошли сюда, значит команда не найдена в таблице
char error_response[128];
snprintf(error_response, sizeof(error_response),
		"{\"status\":\"ERROR\", \"response_to\":\"%s\", \"message\":\"Unknown command: %s\", \"parsing_time_us\":%lu}",
		request_id ? request_id : "unknown", command_name, (unsigned long)parsing_time_us);
Dispatcher_SendUsbResponse(error_response);
}


// --- 4. Создаем "заглушки" для функций-обработчиков ---

/**
 * @brief Обрабатывает простую команду PING
 */
 static void handle_ping(const char* request_id, jsmntok_t *params_token, const char* json_string,
		 int num_tokens, uint32_t parsing_time_us)
 {
	 char response[128];
	 snprintf(response, sizeof(response),
			 "{\"status\":\"OK\", \"response_to\":\"%s\", \"message\":\"PONG\", \"parsing_time_us\":%lu}",
			  request_id ? request_id : "unknown", (unsigned long)parsing_time_us);


	 Dispatcher_SendUsbResponse(response);
}

 /**
  * @brief Обрабатывает сложную команду EXECUTE_JOB
  */
 static void handle_execute_job(const char* request_id, jsmntok_t *params_token, const char* json_string, int num_tokens, uint32_t parsing_time_us)
 {
	 // TODO: Здесь будет сложная логика парсинга параметров из 'params_token'
	 // и заполнения структуры Job_t.
	 // А пока просто отправим подтверждение, что задание получено.
	 char response[128];
	 snprintf(response, sizeof(response),
			 "{\"status\":\"OK\", \"response_to\":\"%s\", \"message\":\"Job received, parsing not yet implemented\","
			 "\"parsing_time_us\":%lu}", request_id ? request_id : "unknown", (unsigned long)parsing_time_us);

	 Dispatcher_SendUsbResponse(response);

}


