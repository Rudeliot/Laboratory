#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

// Структура для обработки подписчиков
typedef struct Observer {
    void (*on_next)(int key_code, const char* event_type);  // Функция обработки нажатия клавиши
    void (*on_complete)();                                   // Функция завершения
    void (*on_error)(const char* error_message);             // Функция обработки ошибок
    struct Observer* next;                                   // Указатель на следующий подписчик
} Observer;

// Структура для трекера событий
typedef struct EventTracker {
    Observer* observers;  // Список подписчиков
    pthread_t thread_id;  // Идентификатор потока
    int stop;             // Флаг завершения работы
} EventTracker;

// Функция для уведомления подписчиков
void notify_observers(EventTracker* tracker, int key_code, const char* event_type) {
    Observer* observer = tracker->observers;
    while (observer != NULL) {
        if (observer->on_next) {
            observer->on_next(key_code, event_type);
        }
        observer = observer->next;
    }
}

// Функция завершения работы
void notify_complete(EventTracker* tracker) {
    Observer* observer = tracker->observers;
    while (observer != NULL) {
        if (observer->on_complete) {
            observer->on_complete();
        }
        observer = observer->next;
    }
}

// Функция обработки ошибок
void notify_error(EventTracker* tracker, const char* error_message) {
    Observer* observer = tracker->observers;
    while (observer != NULL) {
        if (observer->on_error) {
            observer->on_error(error_message);
        }
        observer = observer->next;
    }
}

// Функция для добавления подписчиков
void add_observer(EventTracker* tracker, Observer* observer) {
    observer->next = tracker->observers;
    tracker->observers = observer;
}

// Реализация нажатия клавиши
void* keyboard_event_tracker(void* arg) {
    EventTracker* tracker = (EventTracker*)arg;
    const char* device = "/dev/input/event2";  // Путь к устройству клавиатуры
    int fd = open(device, O_RDONLY);

    if (fd == -1) {
        notify_error(tracker, "Ошибка при открытии устройства");
        return NULL;
    }

    struct input_event ev;
    while (!tracker->stop) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == sizeof(ev)) {
            // Обрабатываем только события с клавишами
            if (ev.type == EV_KEY) {
                const char* event_type = "";
                if (ev.value == 1) {
                    event_type = "Нажата";
                } else if (ev.value == 0) {
                    event_type = "Отпущена";
                } else if (ev.value == 2) {
                    event_type = "Удерживается";  // Зажатая клавиша
                } else {
                    event_type = "Неизвестно";
                }
                notify_observers(tracker, ev.code, event_type);
            }
        } else {
            notify_error(tracker, "Ошибка при чтении события");
        }
    }

    close(fd);
    notify_complete(tracker);
    return NULL;
}

// Функция для создания трекера
EventTracker* create_event_tracker() {
    EventTracker* tracker = (EventTracker*)malloc(sizeof(EventTracker));
    tracker->observers = NULL;
    tracker->stop = 0;
    return tracker;
}

// Функция для завершения трекера
void stop_event_tracker(EventTracker* tracker) {
    tracker->stop = 1;
    pthread_join(tracker->thread_id, NULL);
    free(tracker);
}

// Пример реализации подписчика (вывод в файл)
void file_observer_on_next(int key_code, const char* event_type) {
    FILE* file = fopen("keyboard_events.log", "a");
    if (file != NULL) {
        fprintf(file, "Клавиша: %d, Событие: %s\n", key_code, event_type);
        fclose(file);
    }
}

void file_observer_on_complete() {
    printf("Трекер событий завершил свою работу.\n");
}

void file_observer_on_error(const char* error_message) {
    fprintf(stderr, "Ошибка: %s\n", error_message);
}

int main() {
    EventTracker* tracker = create_event_tracker();

    // Создание и добавление подписчика
    Observer* file_observer = (Observer*)malloc(sizeof(Observer));
    file_observer->on_next = file_observer_on_next;
    file_observer->on_complete = file_observer_on_complete;
    file_observer->on_error = file_observer_on_error;
    add_observer(tracker, file_observer);

    // Запуск трекера событий в отдельном потоке
    if (pthread_create(&tracker->thread_id, NULL, keyboard_event_tracker, tracker) != 0) {
        notify_error(tracker, "Ошибка при создании потока");
        return 1;
    }

    // Завершение работы по сочетанию клавиш
    printf("Нажмите ESC для завершения работы...\n");
    while (!tracker->stop) {
        struct input_event ev;
        int fd = open("/dev/input/event2", O_RDONLY);
        if (fd != -1) {
            ssize_t n = read(fd, &ev, sizeof(ev));
            if (n == sizeof(ev) && ev.type == EV_KEY && ev.code == 1 /* ESC */ && ev.value == 0) {
                stop_event_tracker(tracker);
            }
            close(fd);
        }
    }

    return 0;
}
