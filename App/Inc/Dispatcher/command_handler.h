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
#include <stdbool.h>




/**
 * @brief Определяем универсальный тип для указателя на функцию-обработчик.
 * @param data Указатель на данные, специфичные для команды (например, на Job_t), или NULL.
 * @param start_time Время начала обработки (для измерения производительности).
 */

typedef void (*CommandHandler_t)(void *data, uint32_t start_time);
/**
 * @brief Главная функция, которая находит и выполняет команду.
 * @param command_name Имя команды, полученное из JSON.
 * @param data Указатель на данные для команды.
 * @param start_time Время начала обработки.
 */
void CommandHandler_Execute(const char* command_name, void* data, uint32_t start_time);


#endif /* INC_DISPATCHER_COMMAND_HANDLER_H_ */
