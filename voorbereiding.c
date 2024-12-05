#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep function
#include "MQTTClient.h" // Include Paho MQTT library

#define ADDRESS     "tcp://192.168.68.222:1883"
#define CLIENTID    "Ruben"
#define TOPIC       "receive/topic/ruben"
#define QOS         1
#define TIMEOUT     500L

// Callback for when a message arrives
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    // Get the payload as a null-terminated string
    char *payload = (char *)message->payload;
    char processedMessage[512];

    // Process the message (e.g., prepend "Processed:")
    snprintf(processedMessage, sizeof(processedMessage), "Processed: %s", payload);

    // Log the processed message to file
    FILE *file = fopen("output.log", "a");
    if (file) {
        fprintf(file, "%s\n", processedMessage);
        fclose(file);
    } else {
        perror("Error opening log file");
    }

    // Print the processed message to the console
    printf("%s\n", processedMessage);

    // Free the message object
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return 1;
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // Connect to the MQTT broker
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // Set the callback function
    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);

    // Subscribe to the topic
    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("Subscribed to topic: %s\n", TOPIC);

    // Keep the program running to receive messages
    while (1) {
        sleep(1); // Prevent excessive CPU usage
    }

    // Clean up
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}
