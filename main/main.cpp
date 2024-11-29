#include <Arduino.h>
#include <WiFi.h>

#include <esp_heap_task_info.h>
#include <esp_heap_trace.h>

#define MAX_TASK_NUM 20     // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20    // Max number of per block info that it can store

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void dumpPerTaskHeapInfo() {
    heap_task_info_params_t heapInfo = {
        .caps = { MALLOC_CAP_8BIT, MALLOC_CAP_32BIT },
        .mask = { MALLOC_CAP_8BIT, MALLOC_CAP_32BIT },
        .tasks = nullptr,
        .num_tasks = 0,
        .totals = s_totals_arr,
        .num_totals = &s_prepopulated_num,
        .max_totals = MAX_TASK_NUM,
        .blocks = s_block_arr,
        .max_blocks = MAX_BLOCK_NUM
    };

    heap_caps_get_per_task_info(&heapInfo);

    for (int i = 0; i < *heapInfo.num_totals; i++) {
        auto taskInfo = heapInfo.totals[i];
        std::string taskName = taskInfo.task
            ? pcTaskGetName(taskInfo.task)
            : "Pre-Scheduler allocs";
        taskName.resize(configMAX_TASK_NAME_LEN, ' ');
        printf("Task %p: %s CAP_8BIT: %d, CAP_32BIT: %d\n",
            taskInfo.task,
            taskName.c_str(),
            taskInfo.size[0],
            taskInfo.size[1]);
    }
}

#define NUM_RECORDS 256
static heap_trace_record_t trace_record[NUM_RECORDS];    // This buffer must be in internal RAM

extern "C" void app_main() {
    initArduino();

    ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record, NUM_RECORDS));

    while (true) {
        printf("-----------------\nTotal free heap: %ld\n", ESP.getFreeHeap());
        dumpPerTaskHeapInfo();
        ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));

        printf("Connecting");
        WiFi.begin("Wokwi-GUEST", "", 6);
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            printf(".");
        }
        printf(" done\n");
        delay(1000);

        printf("Disconnecting");
        WiFi.disconnect(true);
        while (WiFi.status() == WL_CONNECTED) {
            delay(250);
            printf(".");
        }
        WiFi.mode(WIFI_OFF);
        printf(" done\n");

        ESP_ERROR_CHECK(heap_trace_stop());
        heap_trace_dump();
        delay(1000);
        printf("\n\n");
    }
}
