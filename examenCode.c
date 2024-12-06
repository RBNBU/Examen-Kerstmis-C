#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADDRESS     "tcp://192.168.0.102:1883"
#define CLIENTID    "RubenClient"
#define TOPIC       "P1/MD4"
#define QOS         1
#define TIMEOUT     50L
#define DATE_TIME_LEN 32
#define PRINT_MSG_LEN 1024

typedef struct { //defenieer de datastructuur
    char datum_tijd_stroom [ DATE_TIME_LEN ];
    int tarief_indicator;
    float actueel_stroomverbruik;
    float actueel_spanning;
    float totaal_dagverbruik;
    float totaal_nachtverbruik;
    float totaal_dagopbrengst;
    float totaal_nachtopbrengst;
    char datum_tijd_gas [ DATE_TIME_LEN ];
    float totaal_gasverbruik;
} DeviceData;

void output_file(const char *message);

void connlost(void *context, char *cause) { //wanneer je de connectie verliest
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
}

void print_device_data(int aantalDagen) {
    printf("\n\n+++++++++++++++++++++++++++++++++++++++++++++++++++++\nElektriciteit- en gas verbruik - totalen per dag\n+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("\nSTARTWAARDEN\n\n");
    printf("DATUM - TIJD: \n");
    printf("Dag Totaal verbruik: \n");
    printf("DAG Totaal opbrengst: \n");
    printf("NACHT Totaal verbruik: \n");
    printf("NACHT Totaal opbrengst \n");
    printf("GAS Totaal verbruik: \n");
    //print waardes totaal

for (int i = 0; i < aantalDagen; ++i){
    printf("\n\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++\nTOTALEN:\n+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Datum: \n");
    printf("STROOM: \n");
    printf("\tTotaal verbruik = \n");
    printf("\tTotaal opbrengst =  \n");
    printf("GAS: \n");
    printf("\tTotaal verbruik = \n");
    //print per dag
}
    printf("\n\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++\nEinde van dit rapport\n+++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
}

void output_file(const char *message) { //write incoming messages to a log file "output.txt"
    FILE *file = fopen("output.txt", "a");
    if (file == NULL) {
        perror("Error opening log file");
        return;
    }
    fprintf(file, "%s\n", message);
    fclose(file);
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) { //zodra er een bericht toekomt
    char *payload = message->payload;
    char *token_str;
    long double totaal_gasverbruik;
    int aantalDagen = 4; //tijdelijk nummer

    token_str = strtok(payload, ";");
    char *datum_tijd_stroom = token_str;
    token_str = strtok(NULL, ";");
    char *tarief_indicator = token_str;
    token_str = strtok(NULL, ";");
    char *actueel_stroomverbruik = token_str;
    token_str = strtok(NULL, ";");
    char *actueel_spanning = token_str;
    token_str = strtok(NULL, ";");
    char *totaal_dagverbruik = token_str;
    token_str = strtok(NULL, ";");
    char *totaal_nachtverbruik = token_str;
    token_str = strtok(NULL, ";");
    char *totaal_dagopbrengst = token_str;
    token_str = strtok(NULL, ";");
    char *totaal_nachtopbrengst = token_str;
    token_str = strtok(NULL, ";");
    char *datum_tijd_gas = token_str;
    token_str = strtok(NULL, ";");
    totaal_gasverbruik = strtold(token_str, NULL);

    if (totaal_gasverbruik == 0) { //wanneer gasverbruik 0 is, detecteer end-of-broadcast en print data
        print_device_data(aantalDagen);
        exit(1);
    }

    printf("Tijd stroom: %s, Tarief: %s, Actueel stroomverbruik: %s, Actuele spanning: %s, Totaal dagverbruik: %s, Totaal nachtverbruik: %s, Totale dagopbrengst: %s, Totale nachtopbrengst: %s, Tijd gas: %s, Totaal gasverbruik: %Lf\n", datum_tijd_stroom, tarief_indicator, actueel_stroomverbruik, actueel_spanning, totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst, datum_tijd_gas, totaal_gasverbruik);
    //print de data in de terminal
    char print_msg[ PRINT_MSG_LEN ];
    sprintf(print_msg, "Tijd stroom: %s, Tarief: %s, Actueel stroomverbruik: %s, Actuele spanning: %s, Totaal dagverbruik: %s, Totaal nachtverbruik: %s, Totale dagopbrengst: %s, Totale nachtopbrengst: %s, Tijd gas: %s, Totaal gasverbruik: %Lf", datum_tijd_stroom, tarief_indicator, actueel_stroomverbruik, actueel_spanning, totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst, datum_tijd_gas, totaal_gasverbruik);
    //zet bericht in print_msg
    output_file(print_msg);
    //zet print_msg in output file

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
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

    while (1) {
        MQTTClient_yield(); //wacht op berichten
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}
