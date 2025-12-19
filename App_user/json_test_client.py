import serial
import json
import time
import sys

# --- Настройки COM-порта ---
# !!! ВАЖНО: замените на имя вашего COM-порта STM32 !!!
# Для Linux/macOS: '/dev/ttyACM0', '/dev/ttyUSB0' и т.д.
# Для Windows: 'COM1', 'COM2' и т.д.
# Если порт не указан в командной строке, используется это значение.
DEFAULT_SERIAL_PORT = '/dev/ttyACM1'
BAUD_RATE = 115200

# --- Пример сложной команды-рецепта (как мы обсуждали) ---
COMPLEX_JOB_COMMAND = {
    "command": "EXECUTE_JOB",
    "request_id": "can_test_1",
    "params": {
        "steps": [
            {
                "action": "SET_PUMP_STATE",
                "params": { "pump_id": 0, "state": 1 }
            }
        ]
    }
}

COMPLEX_JOB_COMMAND1 = {
    "command": "EXECUTE_JOB",
    "request_id": "can_test_1",
    "params": {
        "steps": [
            {
                "action": "SET_PUMP_STATE",
                "params": { "pump_id": 3, "state": 0}
            }
        ]
    }
}

COMPLEX_JOB_COMMAND2= {
    "command": "EXECUTE_JOB",
    "request_id": "can_test_1",
    "params": {
        "steps": [
            {
                "action": "SET_PUMP_STATE",
                "params": { "pump_id": 7, "state": 1}
            }
        ]
    }
}


# --- Простая команда PING ---
PING_COMMAND = {
  "command": "PING",
  "request_id": f"ui_ping_{int(time.time())}"
}


def send_json_command(ser, command_data):
    """
    Отправляет JSON-команду по последовательному порту.
    """
    # Компактный формат без лишних пробелов, чтобы экономить трафик
    json_string = json.dumps(command_data, separators=(',', ':'))
    print(f"Отправка: {json_string}")
    # Отправляем строку в кодировке UTF-8 и добавляем символ новой строки
    ser.write(json_string.encode('utf-8') + b'\n')
    time.sleep(0.1)


def receive_json_response(ser):
    """
    Принимает ответ по последовательному порту и парсит его как JSON.
    """
    try:
        response_line = ser.readline().decode('utf-8').strip()
        if not response_line:
            print("Получен пустой ответ.")
            return None
            
        print(f"Получено: {response_line}")
        response_json = json.loads(response_line)
        return response_json
    except serial.SerialException as e:
        print(f"Ошибка порта при чтении: {e}")
        return None
    except UnicodeDecodeError:
        print("Ошибка декодирования UTF-8. Получены некорректные данные.")
        return None
    except json.JSONDecodeError:
        print("Ошибка декодирования JSON. Ответ не является валидным JSON.")
        return None


if __name__ == "__main__":
    port_name = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SERIAL_PORT

    # 1. Открываем последовательный порт
    try:
        ser = serial.Serial(port_name, BAUD_RATE, timeout=5)
        print(f"Порт {port_name} открыт успешно.")
    except serial.SerialException as e:
        print(f"Ошибка открытия порта {port_name}: {e}")
        print("Доступные порты:")
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        for port, desc, hwid in sorted(ports):
            print(f"- {port}: {desc} [{hwid}]")
        exit()

    time.sleep(2) # Даем STM32 время на инициализацию 

    # 2. Отправляем сложную команду
    print("\n--- Отправляем сложную команду EXECUTE_JOB ---")
    send_json_command(ser, COMPLEX_JOB_COMMAND)
    response = receive_json_response(ser)
    if response:
        print(f"-> Ответ на EXECUTE_JOB: {response}")

    print("\n--- Отправляем сложную команду EXECUTE_JOB ---")
    send_json_command(ser, COMPLEX_JOB_COMMAND1)
    response = receive_json_response(ser)
    if response:
        print(f"-> Ответ на EXECUTE_JOB: {response}")

    print("\n--- Отправляем сложную команду EXECUTE_JOB ---")
    send_json_command(ser, COMPLEX_JOB_COMMAND2)
    response = receive_json_response(ser)
    if response:
        print(f"-> Ответ на EXECUTE_JOB: {response}")

    print("\n--- Отправляем простую команду PING ---")
    send_json_command(ser, PING_COMMAND)
    response = receive_json_response(ser)
    if response:
        print(f"-> Ответ на PING: {response}")

    # 3. Закрываем порт
    ser.close()
    print("\nПорт закрыт.")
