#include "./data_queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

QueueHandle_t global_voltage_queue;
QueueHandle_t global_current_queue;

struct Data_Queue {
    float *buffer;          // 存储 float 数据的缓冲区
    size_t length;          // 最大长度
    size_t head;            // 头指针（出队）
    size_t tail;            // 尾指针（入队）
    size_t count;           // 当前元素个数
    double sum;             // 当前所有元素的和（用于平均值）
};

QueueHandle_t queue_create(size_t length) {
    if (length == 0) return NULL;

    struct Data_Queue *q = (struct Data_Queue*)malloc(sizeof(struct Data_Queue));
    if (!q) return NULL;

    q->buffer = (float*)malloc(sizeof(float) * length);
    if (!q->buffer) {
        free(q);
        return NULL;
    }

    q->length = length;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->sum = 0.0;

    return q;
}

void queue_destroy(QueueHandle_t Data_Queue) {
    if (Data_Queue) {
        free(Data_Queue->buffer);
        free(Data_Queue);
    }
}

int queue_push(QueueHandle_t Data_Queue, float item) {
    if (!Data_Queue) {
        return -1; // 队列未初始化
    }

    // 如果队列已满，先移除最旧的数据
    if (Data_Queue->count == Data_Queue->length) {
        float old_item;
        queue_pop(Data_Queue, &old_item); // 自动移除最旧的数据
    }

    // 添加数据到 tail 位置
    Data_Queue->buffer[Data_Queue->tail] = item;

    // 更新 sum
    Data_Queue->sum += item;

    // 更新 tail
    Data_Queue->tail = (Data_Queue->tail + 1) % Data_Queue->length;
    Data_Queue->count++;

    return 0;
}

int queue_pop(QueueHandle_t Data_Queue, float *item) {
    if (!Data_Queue || !item || Data_Queue->count == 0) {
        return -1; // 队列空
    }

    // 从 head 位置取数据
    *item = Data_Queue->buffer[Data_Queue->head];

    // 更新 sum
    Data_Queue->sum -= *item;

    // 更新 head
    Data_Queue->head = (Data_Queue->head + 1) % Data_Queue->length;
    Data_Queue->count--;

    return 0;
}

double queue_average(QueueHandle_t Data_Queue) {
    if (!Data_Queue || Data_Queue->count == 0) {
        return 0.0;
    }
    return Data_Queue->sum / Data_Queue->count;
}

size_t queue_size(QueueHandle_t Data_Queue) {
    if (!Data_Queue) return 0;
    return Data_Queue->count;
}

size_t queue_capacity(QueueHandle_t Data_Queue) {
    if (!Data_Queue) return 0;
    return Data_Queue->length;
}
