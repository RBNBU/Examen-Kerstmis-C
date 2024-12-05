#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADDRESS     "tcp://192.168.68.222:1883"
#define CLIENTID    "RubenClient"
#define TOPIC       "ruben/receive/topic"
#define QOS         1
#define TIMEOUT     500L

typedef struct {
    char device[50];
    double sum;
    double min;
    double max;
    int count;
} DeviceData;

#define MAX_DEVICES 100

DeviceData devices[MAX_DEVICES];
int device_count = 0;

void update_device_data(char *device, double value) {
    for (int i = 0; i < device_count; ++i) {
        if (strcmp(devices[i].device, device) == 0) {
            devices[i].sum += value;
            if (value < devices[i].min) devices[i].min = value;
            if (value > devices[i].max) devices[i].max = value;
            devices[i].count++;
            return;
        }
    }
    strcpy(devices[device_count].device, device);
    devices[device_count].sum = value;
    devices[device_count].min = value;
    devices[device_count].max = value;
    devices[device_count].count = 1;
    device_count++;
}

void print_device_data() {
    double total_sum = 0.0;
    double global_min = __DBL_MAX__;
    double global_max = -__DBL_MAX__;
    int total_count = 0;

    for (int i = 0; i < device_count; ++i) {
        double avg = devices[i].sum / devices[i].count;
        printf("Device: %s, Min: %.2f, Max: %.2f, Avg: %.2f\n",
               devices[i].device, devices[i].min, devices[i].max, avg);

        total_sum += devices[i].sum;
        if (devices[i].min < global_min) global_min = devices[i].min;
        if (devices[i].max > global_max) global_max = devices[i].max;
        total_count += devices[i].count;
    }

    double global_avg = total_sum / total_count;
    printf("Overall Min: %.2f, Max: %.2f, Avg: %.2f\n", global_min, global_max, global_avg);
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char payload[message->payloadlen + 1];
    snprintf(payload, message->payloadlen + 1, "%s", (char *)message->payload);

    char device[50];
    double value;
    sscanf(payload, "%[^;];%lf", device, &value);

    printf("Received: %s;%.2f\n", device, value);
    FILE *file = fopen("output.txt", "a");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    fprintf(file, "%s;%.2f\n", device, value);
    fclose(file);

    update_device_data(device, value);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, messageArrived, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    MQTTClient_subscribe(client, TOPIC, QOS);

    printf("Press Q<Enter> to quit\n");
    int ch;
    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    print_device_data();

    return rc;
}