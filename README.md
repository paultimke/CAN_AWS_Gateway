# CAN-WIFI Gateway to AWS Cloud

This simple project listens for messages of devices in a CAN bus. When a message arrives, it wraps it in a JSON object and sends it over Wifi with MQTT to an AWS IoT Core cloud. Communication with CAN was made using an MCP2515 as I did not have any standalone transceivers lying around.

To run the project, you need to have an AWS account and configure the IoT "Thing" yourself to get the necessary certificates and keys. When creating the policy, make sure to set all necessary MQTT permissions to be able to publish and subscribe to topics. The topics used by the application are `AWS/esp32_pub` and `AWS/esp32_sub`.

### Configure the project
**TODO:** For now, the AWS MQTT endpoint as well as the wifi SSID and password are hardcoded into a local file on my machine. In the future, I plan to integrate a serial CLI to enable provisioning via UART.

With the current setup, the steps to configure the project with all necessary credentials are:

* Create the AWS Iot Core Thing for your ESP32. Refer to amazon for steps to do this:
https://docs.aws.amazon.com/iot/latest/developerguide/iot-moisture-create-thing.html

* Once you have your certificates and the private key, copy and paste their contents without modifications correspondingly into:
  * `certs/aws_root_ca_pem`
  * `certs/certificate_pem_crt`
  * `certs/private_pem_key`

* Modify the AWS Endpoint (MQTT Host address) named 'HostAddress' in the `modules/aws/aws_iot.c` file
  
* Configure Wi-Fi credentials by modifying the required constants in the `Credentials/wifi_credentials.h` header file.

### Example
This example uses a 2-node CAN bus, where one node sends CAN messages to the Gateway (left-hand ESP32).
![IMG_2634](https://github.com/paultimke/CAN_AWS_Gateway/assets/87957114/52507f51-fb03-4258-af50-cac790745c3f)

<img width="1120" alt="Screen Shot 2024-01-10 at 1 25 00 p m" src="https://github.com/paultimke/CAN_AWS_Gateway/assets/87957114/954bbbc7-afdb-419d-b459-d3a1be31abd2">

