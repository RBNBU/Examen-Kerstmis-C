#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.68.222:1883"
#define CLIENTID    "Ruben"
#define TOPIC       "ruben/receive/topic"
#define QOS         1

// Structure to store statistics for a user/device
typedef struct Stats {
    int min;
    int max;
    double sum;
    int count;
} Stats;

// Node structure for the linked list
typedef struct DeviceNode {
    char device[64];
    Stats stats;
    struct DeviceNode *next;
} DeviceNode;

DeviceNode *head = NULL; // Head of the linked list

// Global overall stats
Stats overallStats = { .min = INT_MAX, .max = INT_MIN, .sum = 0, .count = 0 };

// Function to update statistics
void updateStats(Stats *stats, int value) {
    if (value < stats->min) stats->min = value;
    if (value > stats->max) stats->max = value;
    stats->sum += value;
    stats->count++;
}

// Find or add a device node in the linked list
DeviceNode *getDeviceNode(const char *device) {
    DeviceNode *current = head;

    // Search for the device
    while (current != NULL) {
        if (strcmp(current->device, device) == 0) {
            return current;
        }
        current = current->next;
    }

    // If not found, create a new node
    DeviceNode *newNode = (DeviceNode *)malloc(sizeof(DeviceNode));
    if (!newNode) {
        perror("Failed to allocate memory for new device node");
        exit(EXIT_FAILURE);
    }
    strcpy(newNode->device, device);
    newNode->stats.min = INT_MAX;
    newNode->stats.max = INT_MIN;
    newNode->stats.sum = 0;
    newNode->stats.count = 0;
    newNode->next = head;
    head = newNode;
    return newNode;
}

// Callback for incoming messages
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char *payload = (char *)message->payload;
    payload[message->payloadlen] = '\0'; // Ensure null-terminated string

    // Parse the payload (assume format: "user/device";"value")
    char device[64];
    int value;
    if (sscanf(payload, "\"%63[^\"]\";\"%d\"", device, &value) != 2) {
        printf("Invalid message format: %s\n", payload);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }

    // Update overall statistics
    updateStats(&overallStats, value);

    // Update statistics for the specific device
    DeviceNode *deviceNode = getDeviceNode(device);
    updateStats(&deviceNode->stats, value);

    // Log and print
    FILE *file = fopen("output.log", "a");
    if (file) {
        fprintf(file, "Device: %s, Value: %d\n", device, value);
        fprintf(file, "Overall - Min: %d, Max: %d, Avg: %.2f\n",
                overallStats.min, overallStats.max,
                overallStats.count > 0 ? overallStats.sum / overallStats.count : 0.0);
        fprintf(file, "Device %s - Min: %d, Max: %d, Avg: %.2f\n",
                device, deviceNode->stats.min, deviceNode->stats.max,
                deviceNode->stats.count > 0 ? deviceNode->stats.sum / deviceNode->stats.count : 0.0);
        fclose(file);
    }
    printf("Device: %s, Value: %d\n", device, value);
    printf("Overall - Min: %d, Max: %d, Avg: %.2f\n",
           overallStats.min, overallStats.max,
           overallStats.count > 0 ? overallStats.sum / overallStats.count : 0.0);
    printf("Device %s - Min: %d, Max: %d, Avg: %.2f\n",
           device, deviceNode->stats.min, deviceNode->stats.max,
           deviceNode->stats.count > 0 ? deviceNode->stats.sum / deviceNode->stats.count : 0.0);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

// Free memory allocated for the linked list
void freeDeviceList(DeviceNode *node) {
    while (node) {
        DeviceNode *temp = node;
        node = node->next;
        free(temp);
    }
}

int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect to the broker.\n");
        return EXIT_FAILURE;
    }

    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);
    MQTTClient_subscribe(client, TOPIC, QOS);

    printf("Subscribed to topic: %s\n", TOPIC);

    // Keep the program running
    while (1) {
        pause();
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    freeDeviceList(head);

    return 0;
}