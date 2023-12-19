# CAN-WIFI Gateway to AWS Cloud

This simple project listens for messages of devices in a CAN bus. When a message arrives, it wraps it in a JSON string and sends it over Wifi with MQTT to an AWS IoT Core cloud. Messages can also be sent from the cloud to each device by giving the Device ID inside the JSON string.

To run the project, you need to have an AWS account and configure the IoT "Thing" yourself to get the necessary certificates and keys.

### Configure the project
* Configure Wi-Fi credentials by modifying the required constants in the "Credentials/wifi_credentials.h" header file.
* Configure AWS credentials by modifying the required constants in the "Credentials/aws_credentials.h" header file.

