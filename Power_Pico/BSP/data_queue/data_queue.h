// data_queue.h
#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

#include <stdint.h>
#include <stddef.h>

// 队列句柄（不透明指针）
typedef struct Data_Queue* QueueHandle_t;

/**
 * @brief 创建一个队列
 * @param length: 队列最大长度
 * @return 队列句柄，失败返回 NULL
 */
QueueHandle_t queue_create(size_t length);

/**
 * @brief 销毁队列
 * @param Data_Queue: 队列句柄
 */
void queue_destroy(QueueHandle_t Data_Queue);

/**
 * @brief 入队（Push）
 * @param Data_Queue: 队列句柄
 * @param item: 指向要入队的数据
 * @return 0=成功，-1=失败（队列满）
 */
int queue_push(QueueHandle_t Data_Queue, float item);

/**
 * @brief 出队（Pop）
 * @param Data_Queue: 队列句柄
 * @param item: 指向接收数据的缓冲区
 * @return 0=成功，-1=失败（队列空）
 */
int queue_pop(QueueHandle_t Data_Queue, float *item);

/**
 * @brief 获取队列中当前元素的平均值（仅适用于数值类型）
 * @param Data_Queue: 队列句柄
 * @return 平均值，若队列空则返回 0.0
 */
double queue_average(QueueHandle_t Data_Queue);

/**
 * @brief 获取队列当前大小
 */
size_t queue_size(QueueHandle_t Data_Queue);

/**
 * @brief 获取队列最大容量
 */
size_t queue_capacity(QueueHandle_t Data_Queue);

extern QueueHandle_t global_voltage_queue;
extern QueueHandle_t global_current_queue;

#endif // DATA_QUEUE_H
