/*
 * job_format.h
 *
 *  Created on: Dec 16, 2025
 *      Author: andrey
 */

#ifndef INC_JOB_FORMAT_H_
#define INC_JOB_FORMAT_H_


#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

// --- Структуры для параметров каждого действия ---
typedef struct {
	uint8_t motor_id;
	int32_t position;
    uint16_t speed;
} MoveParams_t;

typedef struct {
	uint8_t pump_id;
	bool    state;
	uint32_t duration_ms;
} PumpParams_t;

typedef struct {
	uint8_t sensor_id;
} GetTempParams_t;

typedef struct {
	uint32_t duration_ms;
} WaitParams_t;

// --- Основные структуры для задания ---
// Структура, описывающая один шаг в "рецепте"

typedef struct {
	ActionType_t action;
	union { // Используем union для экономии памяти
		MoveParams_t move;
		PumpParams_t pump;
		GetTempParams_t temp;
		WaitParams_t wait;
		} params;
} JobStep_t;



// Главная структура, описывающая весь "рецепт" (задание)
typedef struct {
	char job_name[64];
	char request_id[32];
	JobStep_t steps[MAX_STEPS_PER_JOB];
	uint8_t num_steps;
} Job_t;



#endif /* INC_JOB_FORMAT_H_ */
