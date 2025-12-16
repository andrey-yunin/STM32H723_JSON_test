/*
 * command_handler.h
 *
 *  Created on: Dec 16, 2025
 *      Author: andrey
 */

#ifndef INC_DISPATCHER_COMMAND_HANDLER_H_
#define INC_DISPATCHER_COMMAND_HANDLER_H_

#include "jsmn.h" // Для типа jsmntok_t
#include <stdint.h>

// Определяем тип для указателя на функцию-обработчик.
// Все наши обработчики будут принимать одни и те же аргументы:
// - request_id: ID запроса от UI
// - params_token: Указатель на токен, с которого начинаются параметры в JSON
// - json_string: Указатель на всю JSON-строку
// - num_tokens: Общее количество токенов
typedef void (*CommandHandler_t)(const char* request_id, jsmntok_t *params_token, const char* json_string, int num_tokens, uint32_t parsing_time_us);

/**
 * @brief Главная функция, которая находит и выполняет команду.
 *
 * @param command_name Имя команды, полученное из JSON.
 * @param request_id ID запроса, полученный из JSON.
 * @param params_token Указатель на токен параметров.
 * @param json_string Указатель на всю JSON-строку.
 * @param num_tokens Общее количество токенов.
 */
void CommandHandler_Execute(const char* command_name, const char* request_id, jsmntok_t *params_token, const char* json_string, int num_tokens, uint32_t parsing_time_us);

#endif /* INC_DISPATCHER_COMMAND_HANDLER_H_ */
